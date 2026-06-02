#include "games/rogue/hooks/fps_unlock.hpp"

#include <cstdint>

#include <algorithm>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"
#include "core/mem/x64.hpp"

#include "games/rogue/registry.hpp"
#include "games/rogue/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::FPSUnlockHook;

        games::rogue::FrameTiming *g_frame_timing = nullptr;

        constexpr float         k_min_fps       = 1.0F;
        constexpr std::uint32_t k_mode_fixed    = 0;
        constexpr std::uint32_t k_mode_averaged = 3;

        void apply_fps_patch(float target) {
            if (g_frame_timing == nullptr) {
                return;
            }

            target = std::max(target, 0.0F);

            if (target < k_min_fps) {
                g_frame_timing->timing_mode = k_mode_averaged;
                g_frame_timing->target_time = 0;
                log::get()->trace("FPSUnlockHook: uncapped (mode=averaged, target_time zeroed)");
            } else {
                g_frame_timing->timing_mode = k_mode_fixed;
                g_frame_timing->fixed_rate  = target;
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

        apply_fps_patch(games::rogue::registry().config<Tag>().target.get());

        log::get()->trace("FPSUnlockHook: installed");
        return true;
    }
} // namespace hooks
