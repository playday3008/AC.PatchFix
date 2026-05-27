#include "hooks/common/fps_unlock.hpp"

#include <cstdint>

#include <algorithm>

#include <injector/injector.hpp>

#include "logger.hpp" // IWYU pragma: keep

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"

namespace hooks {
    namespace {
        using G   = games::Rogue;
        using Tag = FPSUnlockHook<G>;

        uintptr_t g_sleep_branch_addr   = 0;
        uintptr_t g_frame_time_addr     = 0;
        float     g_original_frame_time = 15.6666F;

        constexpr uint8_t k_jnb_opcode = 0x73;
        constexpr uint8_t k_jmp_opcode = 0xEB;
        constexpr float   k_min_fps    = 1.0F;

        void apply_fps_patch(float target) {
            target = std::max(target, 0.0F);

            if (target < k_min_fps) {
                injector::WriteMemory<uint8_t>(g_sleep_branch_addr, k_jmp_opcode, true);
                injector::WriteMemory<float>(g_frame_time_addr, g_original_frame_time, true);
                log::get()->trace("FPSUnlockHook: uncapped");
            } else {
                injector::WriteMemory<uint8_t>(g_sleep_branch_addr, k_jnb_opcode, true);
                injector::WriteMemory<float>(g_frame_time_addr, 1000.0F / target, true);
                log::get()->trace("FPSUnlockHook: capped to {:.1f} FPS ({:.4f} ms)",
                                  target,
                                  1000.0F / target);
            }
        }
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        float target = cfg.target.get();
        log::get()->trace("FPSUnlockHook: on_reload target={}", target);
        apply_fps_patch(target);
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        g_sleep_branch_addr = addrs.fps_sleep_branch.value();
        log::get()->trace("FPSUnlockHook: sleep branch at 0x{:X}", g_sleep_branch_addr);

        auto mulss_addr   = addrs.fps_frame_time.value();
        g_frame_time_addr = injector::ReadRelativeOffset(mulss_addr + 4, 4).as_int();
        log::get()->trace("FPSUnlockHook: frame time constant at 0x{:X}", g_frame_time_addr);

        g_original_frame_time = injector::ReadMemory<float>(g_frame_time_addr, true);
        log::get()->trace("FPSUnlockHook: original frame time = {:.4f} ms ({:.1f} FPS)",
                          g_original_frame_time,
                          1000.0F / g_original_frame_time);

        apply_fps_patch(games::rogue::g_registry.config<Tag>().target.get());

        log::get()->trace("FPSUnlockHook: installed");
        return true;
    }
} // namespace hooks
