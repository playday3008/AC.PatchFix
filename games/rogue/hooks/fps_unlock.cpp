#include "games/rogue/hooks/fps_unlock.hpp"

#include <cstdint>

#include <algorithm>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"
#include "core/mem/x64.hpp"
#include "core/win32/timer_resolution.hpp"

#include "games/rogue/registry.hpp"
#include "games/rogue/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::FPSUnlockHook;

        games::rogue::FrameTiming *g_frame_timing = nullptr;

        // The frame deadline period in milliseconds, read by the mulss inside
        // UpdateFrameTiming. Sole reference in the binary, so it is ours to retune.
        std::uintptr_t g_cap_period = 0;

        constexpr float         k_min_fps    = 1.0F;
        constexpr std::uint32_t k_mode_vsync = 2;

        void apply_fps_patch(float target) {
            if (g_frame_timing == nullptr || g_cap_period == 0) {
                return;
            }

            target = std::max(target, 0.0F);

            // In vsync mode the deadline is current_time + trunc(ticks_per_ms *
            // period), so the period alone paces the game and the engine keeps its
            // own measured delta. A period of zero leaves the deadline at "now",
            // which the wait loop never has to wait for.
            float period = target < k_min_fps ? 0.0F : 1000.0F / target;

            // Only the capped path sleeps. Raising the process timer resolution
            // while uncapped changes Sleep() granularity for every other thread
            // in the game and paces nothing.
            if (period == 0.0F) {
                win32::restore_timer_resolution();
            } else if (win32::raise_timer_resolution()) {
                log::get()->trace("FPSUnlockHook: timer resolution raised to 1 ms");
            } else {
                log::get()->warn("FPSUnlockHook: failed to raise timer resolution");
            }

            if (!mem::write<float>(g_cap_period, period)) {
                log::get()->error("FPSUnlockHook: failed to write cap period");
                return;
            }

            // The pending deadline was computed from the previous period and would
            // stall the wait loop once before the next frame recomputes it.
            g_frame_timing->target_time = 0;

            if (period == 0.0F) {
                log::get()->trace("FPSUnlockHook: uncapped (period=0.0)");
            } else {
                log::get()->trace("FPSUnlockHook: capped to {:.1f} FPS (period={:.4f} ms)",
                                  target,
                                  period);
            }
        }
    } // namespace

    void HookTraits<games::rogue::FPSUnlockHook>::on_reload(const Config &cfg) {
        float target = cfg.target.get();
        log::get()->trace("FPSUnlockHook: on_reload target={}", target);
        apply_fps_patch(target);
    }

    auto HookTraits<games::rogue::FPSUnlockHook>::install(const Addrs &addrs) -> bool {
        auto thunk_addr  = addrs.fps_timing_ptr.value();
        auto global_addr = mem::x64::read_rel(thunk_addr + 3);
        log::get()->trace("FPSUnlockHook: FrameTiming global at 0x{:X}", global_addr);

        auto ft_ptr = mem::read<std::uintptr_t>(global_addr);
        if (ft_ptr == 0) {
            log::get()->error("FPSUnlockHook: FrameTiming pointer is null");
            return false;
        }

        g_frame_timing = reinterpret_cast<games::rogue::FrameTiming *>(ft_ptr);
        log::get()->trace("FPSUnlockHook: FrameTiming instance at 0x{:X} "
                          "(timing_mode={}, fixed_rate={:.1f})",
                          ft_ptr,
                          g_frame_timing->timing_mode,
                          g_frame_timing->fixed_rate);

        // The constructor sets vsync mode and nothing in the game writes the field
        // afterwards. Any other value means the period below paces nothing.
        if (g_frame_timing->timing_mode != k_mode_vsync) {
            log::get()->warn("FPSUnlockHook: unexpected timing_mode={}, expected {}",
                             g_frame_timing->timing_mode,
                             k_mode_vsync);
        }

        g_cap_period = mem::x64::read_rel(addrs.fps_cap_mulss.value() + 4);
        log::get()->trace("FPSUnlockHook: cap period at 0x{:X} ({:.6f} ms)",
                          g_cap_period,
                          mem::read<float>(g_cap_period));

        apply_fps_patch(games::rogue::registry().config<Tag>().target.get());

        log::get()->trace("FPSUnlockHook: installed");
        return true;
    }
} // namespace hooks
