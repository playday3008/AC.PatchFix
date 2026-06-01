#include "games/syndicate/hooks/refresh_rate_fix.hpp"

#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/syndicate/registry.hpp"
#include "games/syndicate/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::RefreshRateFixHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        struct FixRefreshRate {
            static constexpr std::string_view name = "RefreshRateFix";
            [[maybe_unused]] static void      operator()(mem::Registers &regs) {
                auto mode = static_cast<int>(regs.r9);
                if (mode != 2) {
                    return;
                }

                auto *obj = reinterpret_cast<games::syndicate::SwapChainObj *>(regs.rcx);
                obj->mode_desc.refresh_rate.numerator   = 0;
                obj->mode_desc.refresh_rate.denominator = 0;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate RefreshRateFixHook: installing");

        const auto &cfg = games::syndicate::registry().config<Tag>();
        if (!cfg.enabled.get()) {
            log::get()->info("Syndicate RefreshRateFixHook: disabled, skipping");
            return true;
        }

        auto addr = addrs.swapchain_resize.value();
        log::get()->trace(
            "Syndicate RefreshRateFixHook: SwapChain_ResizeAndSetFullscreen at 0x{:X}",
            addr);

        auto hook_result = mem::make_hook<FixRefreshRate>(addr);
        if (!hook_result) {
            log::get()->error("Syndicate RefreshRateFixHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("Syndicate RefreshRateFixHook: installed");
        return true;
    }
} // namespace hooks
