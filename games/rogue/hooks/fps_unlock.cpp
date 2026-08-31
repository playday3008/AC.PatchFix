#include "games/rogue/hooks/fps_unlock.hpp"

#include <cstdint>

#include <algorithm>

#include <Windows.h>

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

        // The vsync-mode deadline period in milliseconds, read by the mulss inside
        // UpdateFrameTiming. Sole reference in the binary, so it is ours to retune.
        std::uintptr_t g_cap_period      = 0;
        float          g_cap_period_orig = 0.0F;

        constexpr float         k_min_fps    = 1.0F;
        constexpr std::uint32_t k_mode_fixed = 0;
        constexpr std::uint32_t k_mode_vsync = 2;

        void prime_timestamps() {
            LARGE_INTEGER qpc;
            QueryPerformanceCounter(&qpc);
            auto now                      = static_cast<std::uint64_t>(qpc.QuadPart);
            g_frame_timing->current_time  = now;
            g_frame_timing->previous_time = now;
            g_frame_timing->target_time   = 0;
        }

        void apply_fps_patch(float target) {
            if (g_frame_timing == nullptr || g_cap_period == 0) {
                return;
            }

            target = std::max(target, 0.0F);

            if (target < k_min_fps) {
                // Vsync mode measures each frame individually and keeps the engine's
                // own 1/15 s clamp on the delta. Zeroing its period leaves the frame
                // deadline at "now", so the wait loop never runs.
                if (!mem::write<float>(g_cap_period, 0.0F)) {
                    log::get()->error("FPSUnlockHook: failed to zero cap period");
                    return;
                }
                g_frame_timing->timing_mode = k_mode_vsync;
                prime_timestamps();
                log::get()->trace("FPSUnlockHook: uncapped (mode=vsync, period=0.0)");
            } else {
                if (!mem::write<float>(g_cap_period, g_cap_period_orig)) {
                    log::get()->error("FPSUnlockHook: failed to restore cap period");
                    return;
                }
                g_frame_timing->timing_mode = k_mode_fixed;
                g_frame_timing->fixed_rate  = target;
                prime_timestamps();
                log::get()->trace("FPSUnlockHook: capped to {:.1f} FPS (mode=fixed, "
                                  "fixed_rate={:.4f})",
                                  target,
                                  target);
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

        g_cap_period      = mem::x64::read_rel(addrs.fps_cap_mulss.value() + 4);
        g_cap_period_orig = mem::read<float>(g_cap_period);
        log::get()->trace("FPSUnlockHook: cap period at 0x{:X} ({:.6f} ms)",
                          g_cap_period,
                          g_cap_period_orig);

        // The game never raises the timer resolution, so its Sleep(1) frame wait
        // runs at the ~15.6 ms default quantum and cannot pace anything faster
        // than ~64 FPS.
        if (win32::raise_timer_resolution()) {
            log::get()->trace("FPSUnlockHook: timer resolution raised to 1 ms");
        } else {
            log::get()->warn("FPSUnlockHook: failed to raise timer resolution");
        }

        apply_fps_patch(games::rogue::registry().config<Tag>().target.get());

        log::get()->trace("FPSUnlockHook: installed");
        return true;
    }
} // namespace hooks
