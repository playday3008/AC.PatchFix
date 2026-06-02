#include "games/syndicate/hooks/prompt_override.hpp"

#include <cstdint>

#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/syndicate/registry.hpp"
#include "games/syndicate/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::PromptOverrideHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        std::uintptr_t g_ret_addr = 0;

        struct OverrideDeviceType {
            [[maybe_unused]] static constexpr std::string_view name = "PromptOverride";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                const auto *ctx =
                    reinterpret_cast<const games::syndicate::InputContext *>(regs.rcx);
                if (ctx == nullptr || ctx->state == nullptr ||
                    ctx->state->device_manager == nullptr) {
                    return;
                }
                const auto &slot = ctx->state->device_manager->active_slot();

                if (slot.device_type == 0) {
                    return;
                }

                if (!games::syndicate::registry().enabled<Tag>()) {
                    return;
                }

                const auto &cfg = games::syndicate::registry().config<Tag>();
                regs.rax        = std::to_underlying(cfg.type.get());
                regs.rip        = g_ret_addr;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate PromptOverrideHook: installing");

        auto fn_addr = addrs.get_active_device_type.value();
        g_ret_addr   = fn_addr + 0x1F;

        log::get()->trace("PromptOverrideHook: get_active_device_type at 0x{:X}, ret at 0x{:X}",
                          fn_addr,
                          g_ret_addr);

        auto hook_result = mem::make_hook<OverrideDeviceType>(fn_addr);
        if (!hook_result) {
            log::get()->error("PromptOverrideHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        const auto &cfg = games::syndicate::registry().config<Tag>();
        log::get()->info("PromptOverrideHook: installed (type={})",
                         std::to_underlying(cfg.type.get()));
        return true;
    }
} // namespace hooks
