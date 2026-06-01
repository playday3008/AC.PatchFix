#include "games/rogue/hooks/fps_unlock.hpp"

#include <cstddef>
#include <cstdint>

#include <algorithm>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/write.hpp"
#include "mem/x64.hpp"

#include "games/rogue/registry.hpp"
#include "games/rogue/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::FPSUnlockHook;
        using games::rogue::FrameTiming;

        std::uintptr_t g_frame_timing_addr = 0;

        constexpr float k_min_fps    = 1.0F;
        constexpr float k_uncap_rate = 1000000.0F;

        void apply_fps_rate(float target) {
            if (g_frame_timing_addr == 0) {
                return;
            }
            target = std::max(target, 0.0F);

            const auto rate_addr = g_frame_timing_addr + offsetof(FrameTiming, fixed_rate);

            if (target < k_min_fps) {
                if (!mem::write<float>(rate_addr, k_uncap_rate)) {
                    log::get()->error("FPSUnlockHook: failed to write uncap rate");
                }
                log::get()->trace("FPSUnlockHook: uncapped (fixed_rate={})", k_uncap_rate);
            } else {
                if (!mem::write<float>(rate_addr, target)) {
                    log::get()->error("FPSUnlockHook: failed to write cap rate");
                }
                log::get()->trace("FPSUnlockHook: capped to {:.1f} FPS", target);
            }
        }
    } // namespace

    void HookTraits<games::rogue::FPSUnlockHook>::on_reload(const Config &cfg) {
        float target = cfg.target.get();
        log::get()->trace("FPSUnlockHook: on_reload target={}", target);
        apply_fps_rate(target);
    }

    auto HookTraits<games::rogue::FPSUnlockHook>::install(const Addrs &addrs) -> bool {
        auto pattern_addr    = addrs.frame_timing_global.value();
        auto global_var_addr = mem::x64::read_rel(pattern_addr + 3);
        auto ft_ptr          = mem::read<std::uintptr_t>(global_var_addr);

        log::get()->trace("FPSUnlockHook: FrameTiming global at 0x{:X}, instance at 0x{:X}",
                          global_var_addr,
                          ft_ptr);

        if (ft_ptr == 0) {
            log::get()->error("FPSUnlockHook: FrameTiming not yet allocated");
            return false;
        }

        g_frame_timing_addr = ft_ptr;

        auto original_mode = mem::read<std::uint32_t>(ft_ptr + offsetof(FrameTiming, timing_mode));
        auto original_rate = mem::read<float>(ft_ptr + offsetof(FrameTiming, fixed_rate));
        log::get()->trace("FPSUnlockHook: original timing_mode={}, fixed_rate={:.1f}",
                          original_mode,
                          original_rate);

        if (!mem::write<std::uint32_t>(ft_ptr + offsetof(FrameTiming, timing_mode), 0)) {
            log::get()->error("FPSUnlockHook: failed to write timing_mode");
            return false;
        }

        apply_fps_rate(games::rogue::registry().config<Tag>().target.get());

        log::get()->trace("FPSUnlockHook: installed (timing_mode=0, struct-based)");
        return true;
    }
} // namespace hooks
