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

        // The frame deadline period in milliseconds, read by the mulss inside
        // UpdateFrameTiming. Sole reference in the binary, so it is ours to retune.
        std::uintptr_t g_cap_period = 0;

        constexpr float         k_min_fps      = 1.0F;
        constexpr std::uint32_t k_mode_vsync   = 2;
        constexpr int           k_mode_default = -1;
        constexpr int           k_mode_max     = 3;

        // Modes 0, 1 and 3 derive the next deadline from the measured frame delta.
        // A stale previous_time makes that delta enormous and parks the deadline
        // hours ahead, which the wait loop never reaches.
        void prime_timestamps() {
            LARGE_INTEGER now {};
            QueryPerformanceCounter(&now);
            auto ticks                    = static_cast<std::uint64_t>(now.QuadPart);
            g_frame_timing->current_time  = ticks;
            g_frame_timing->previous_time = ticks;
            g_frame_timing->target_time   = 0;
        }

        // Diagnostic only. Forcing a mode lets the engine run on a frame delta the
        // hardware cannot actually produce: mode 0 takes dt straight from
        // fixed_rate, mode 1 quantises it to the 15..120 ladder, mode 3 averages
        // the last 15 frames.
        void apply_timing_override(int mode, float rate) {
            if (g_frame_timing == nullptr) {
                return;
            }

            if (rate > 0.0F) {
                g_frame_timing->fixed_rate = rate;
                log::get()->warn("FPSUnlockHook: DIAGNOSTIC fixed_rate={:.1f} (dt={:.4f} ms)",
                                 rate,
                                 1000.0F / rate);
            }

            if (mode == k_mode_default) {
                return;
            }

            if (mode < 0 || mode > k_mode_max) {
                log::get()->error("FPSUnlockHook: TimingMode={} out of range, ignored", mode);
                return;
            }

            auto forced                 = static_cast<std::uint32_t>(mode);
            g_frame_timing->timing_mode = forced;
            prime_timestamps();

            // Every forced mode paces itself through the wait loop, and that loop
            // sleeps in 1 ms steps.
            if (forced != k_mode_vsync) {
                win32::raise_timer_resolution();
            }

            log::get()->warn("FPSUnlockHook: DIAGNOSTIC timing_mode={}", mode);
        }

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
        apply_timing_override(cfg.timing_mode.get(), cfg.fixed_rate.get());
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

        const auto &cfg = games::rogue::registry().config<Tag>();
        apply_fps_patch(cfg.target.get());
        apply_timing_override(cfg.timing_mode.get(), cfg.fixed_rate.get());

        log::get()->trace("FPSUnlockHook: installed");
        return true;
    }
} // namespace hooks
