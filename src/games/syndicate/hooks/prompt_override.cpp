#include "games/syndicate/hooks/prompt_override.hpp"

#include <cstdint>

#include <atomic>
#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/syndicate/registry.hpp"
#include "games/syndicate/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::PromptOverrideHook;

        constexpr std::uintptr_t k_device_kind_offset = 0x790;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        std::atomic<std::uint32_t> g_original_kind {0};
        std::atomic<bool>          g_kind_saved {false};

        struct OverrideDeviceType {
            [[maybe_unused]] static constexpr std::string_view name = "PromptOverride";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                const auto *ctx =
                    reinterpret_cast<const games::syndicate::InputContext *>(regs.rcx);
                auto       *mgr  = ctx->state->device_manager;
                const auto &slot = mgr->active_slot();

                if (slot.device_type == 0) {
                    return;
                }

                // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
                auto *kind_ptr = reinterpret_cast<std::uint32_t *>(
                    static_cast<char *>(slot.device_obj) + k_device_kind_offset);
#pragma clang diagnostic pop
                // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

                if (!g_kind_saved.load(std::memory_order_relaxed)) {
                    g_original_kind.store(*kind_ptr, std::memory_order_relaxed);
                    g_kind_saved.store(true, std::memory_order_relaxed);
                }

                const auto &cfg = games::syndicate::registry().config<Tag>();
                if (!cfg.enabled.get()) {
                    *kind_ptr = g_original_kind.load(std::memory_order_relaxed);
                    return;
                }

                *kind_ptr = static_cast<std::uint32_t>(std::to_underlying(cfg.type.get()));
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate PromptOverrideHook: installing");

        auto fn_addr = addrs.get_active_device_type.value();

        log::get()->trace("Syndicate PromptOverrideHook: get_active_device_type at 0x{:X}",
                          fn_addr);

        auto hook_result = mem::make_hook<OverrideDeviceType>(fn_addr);
        if (!hook_result) {
            log::get()->error("Syndicate PromptOverrideHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        const auto &cfg = games::syndicate::registry().config<Tag>();
        log::get()->info("Syndicate PromptOverrideHook: installed (enabled={}, type={})",
                         cfg.enabled.get(),
                         std::to_underlying(cfg.type.get()));
        return true;
    }
} // namespace hooks
