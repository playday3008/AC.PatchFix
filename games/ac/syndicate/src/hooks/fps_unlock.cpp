#include "games/ac/syndicate/hooks/fps_unlock.hpp"

#include <cstdint>
#include <cstring>

#include <algorithm>
#include <array>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"
#include "core/mem/x64.hpp"

#include "games/ac/syndicate/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::ac::syndicate::FPSUnlockHook;

        std::uintptr_t g_sleep_branch_addr   = 0;
        std::uintptr_t g_flag_gate_addr      = 0;
        std::uintptr_t g_frame_time_addr     = 0;
        float          g_original_frame_time = 32.3333F;

        constexpr std::uint8_t k_jnb_opcode    = 0x73;
        constexpr std::uint8_t k_jmp_opcode    = 0xEB;
        constexpr std::size_t  k_flag_gate_len = 6;
        constexpr float        k_min_fps       = 1.0F;

        std::array<std::uint8_t, k_flag_gate_len> g_original_flag_gate {};

        // The three writes are independent patch sites. Short-circuiting on the
        // first failure would leave a mix of the capped and uncapped states, which
        // is a configuration neither path intends, so each is attempted regardless.
        void apply_fps_patch(float target) {
            target = std::max(target, 0.0F);

            const bool uncap = target < k_min_fps;

            const bool gate_ok =
                uncap ? mem::write(g_flag_gate_addr, g_original_flag_gate.data(), k_flag_gate_len)
                      : mem::nop(g_flag_gate_addr, k_flag_gate_len);
            const bool branch_ok =
                mem::write<std::uint8_t>(g_sleep_branch_addr, uncap ? k_jmp_opcode : k_jnb_opcode);
            const bool time_ok =
                mem::write<float>(g_frame_time_addr,
                                  uncap ? g_original_frame_time : 1000.0F / target);

            if (!gate_ok || !branch_ok || !time_ok) {
                log::get()->error("FPSUnlockHook: failed to write {} patch "
                                  "(flag gate {}, sleep branch {}, frame time {})",
                                  uncap ? "uncap" : "cap",
                                  gate_ok ? "ok" : "FAILED",
                                  branch_ok ? "ok" : "FAILED",
                                  time_ok ? "ok" : "FAILED");
                return;
            }

            if (uncap) {
                log::get()->trace("FPSUnlockHook: uncapped (restored flag gate)");
            } else {
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
        g_flag_gate_addr    = g_sleep_branch_addr - 10;
        log::get()->trace("FPSUnlockHook: sleep branch at 0x{:X}, flag gate at 0x{:X}",
                          g_sleep_branch_addr,
                          g_flag_gate_addr);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
        std::memcpy(g_original_flag_gate.data(),
                    reinterpret_cast<const void *>(g_flag_gate_addr),
                    k_flag_gate_len);
#pragma clang diagnostic pop

        auto mulss_addr   = addrs.fps_frame_time.value();
        g_frame_time_addr = mem::x64::read_rel(mulss_addr + 4, 4);
        log::get()->trace("FPSUnlockHook: frame time constant at 0x{:X}", g_frame_time_addr);

        g_original_frame_time = mem::read<float>(g_frame_time_addr);
        log::get()->trace("FPSUnlockHook: original frame time = {:.4f} ms ({:.1f} FPS)",
                          g_original_frame_time,
                          1000.0F / g_original_frame_time);

        apply_fps_patch(games::ac::syndicate::registry().config<Tag>().target.get());

        log::get()->trace("FPSUnlockHook: installed");
        return true;
    }
} // namespace hooks
