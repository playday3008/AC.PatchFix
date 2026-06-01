#include "games/syndicate/hooks/prompt_override.hpp"

#include <cstdint>

#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/syndicate/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::PromptOverrideHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        std::uintptr_t g_ret_addr    = 0;
        std::uint32_t  g_forced_type = 0;

        struct OverrideDeviceType {
            static constexpr std::string_view name = "PromptOverride";
            [[maybe_unused]] static void      operator()(mem::Registers &regs) {
                // Walk the same pointer chain as the original function:
                //   obj       = *(rcx + 0x10)
                //   dev_mgr   = *(obj + 0x08)
                //   index     = *(dev_mgr + 0x798)
                //   dev_array = *(dev_mgr + 0x79C)
                //   type      = *(dev_array + 0x238*index + 0x08)
                auto obj       = *reinterpret_cast<std::uintptr_t *>(regs.rcx + 0x10);
                auto dev_mgr   = *reinterpret_cast<std::uintptr_t *>(obj + 0x08);
                auto index     = *reinterpret_cast<std::uint32_t *>(dev_mgr + 0x798);
                auto dev_array = *reinterpret_cast<std::uintptr_t *>(dev_mgr + 0x79C);
                auto type =
                    *reinterpret_cast<std::uint32_t *>(dev_array + (0x238ULL * index) + 0x08);

                if (type == 0) {
                    return;
                }

                regs.rax = g_forced_type;
                regs.rip = g_ret_addr;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate PromptOverrideHook: installing");

        const auto &cfg = games::syndicate::registry().config<Tag>();
        if (!cfg.enabled.get()) {
            log::get()->info("Syndicate PromptOverrideHook: disabled, skipping");
            return true;
        }

        auto fn_addr  = addrs.get_active_device_type.value();
        g_ret_addr    = fn_addr + 0x1F;
        g_forced_type = static_cast<std::uint32_t>(std::to_underlying(cfg.type.get()));

        log::get()->trace("Syndicate PromptOverrideHook: get_active_device_type at 0x{:X}",
                          fn_addr);
        log::get()->trace("Syndicate PromptOverrideHook: forcing type={}", g_forced_type);

        auto hook_result = mem::make_hook<OverrideDeviceType>(fn_addr);
        if (!hook_result) {
            log::get()->error("Syndicate PromptOverrideHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("Syndicate PromptOverrideHook: installed — forcing device type {}",
                         g_forced_type);
        return true;
    }
} // namespace hooks
