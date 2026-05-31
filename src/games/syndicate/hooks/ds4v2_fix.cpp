#include "games/syndicate/hooks/ds4v2_fix.hpp"

#include <cstdint>

#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"
#include "mem/write.hpp"

#include "games/syndicate/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::DS4v2FixHook;

        constexpr std::uint16_t k_ds4_v1_pid = 0x05C4;
        constexpr std::uint16_t k_ds4_v2_pid = 0x09CC;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        std::uintptr_t g_skip_target = 0;

        struct CheckDS4PID {
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto pid = static_cast<std::uint16_t>(regs.rax & 0xFFFF);

                if (pid == k_ds4_v1_pid || pid == k_ds4_v2_pid) {
                    log::get()->trace("DS4v2FixHook: DS4 controller detected (PID=0x{:04X})", pid);
                    return;
                }

                regs.rip = g_skip_target;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate DS4v2FixHook: installing");

        const auto &cfg = games::syndicate::registry().config<Tag>();
        if (!cfg.enabled.get()) {
            log::get()->info("Syndicate DS4v2FixHook: disabled, skipping");
            return true;
        }

        auto match_addr = addrs.ds4_type_classify.value();
        log::get()->trace("Syndicate DS4v2FixHook: DS4 PID check at 0x{:X}", match_addr);

        // Read the original jnz displacement to compute skip target
        // jnz is at match_addr + 8, displacement is 1 byte at match_addr + 9
        auto jnz_disp = mem::read<std::int8_t>(match_addr + 9);
        g_skip_target = match_addr + 10 + static_cast<std::uintptr_t>(jnz_disp);
        log::get()->trace("Syndicate DS4v2FixHook: skip target at 0x{:X}", g_skip_target);

        // NOP the original mov ecx + cmp ax,cx + jnz (10 bytes)
        // and hook at the start. The mov [rbx+790h],0Bh at match_addr+10
        // remains intact — our hook falls through to it on PID match.
        auto hook_result = mem::make_hook<CheckDS4PID>(match_addr, match_addr + 10);
        if (!hook_result) {
            log::get()->error("Syndicate DS4v2FixHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("Syndicate DS4v2FixHook: installed — DS4 v2 (PID 0x09CC) now recognized");
        return true;
    }
} // namespace hooks
