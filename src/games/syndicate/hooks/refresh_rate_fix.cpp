#include "games/syndicate/hooks/refresh_rate_fix.hpp"

#include <cstdint>

#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/syndicate/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::RefreshRateFixHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        // The swap chain object stores the current DXGI_MODE_DESC at obj+0xD0.
        // RefreshRate (DXGI_RATIONAL) is at DXGI_MODE_DESC+16 = obj+0xE0.
        constexpr std::uintptr_t k_mode_desc_refresh_offset = 0xE0;

        struct FixRefreshRate {
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto mode = static_cast<int>(regs.r9);
                if (mode != 2) {
                    return;
                }

                auto  obj = regs.rcx;
                auto *numerator =
                    reinterpret_cast<std::uint32_t *>(obj + k_mode_desc_refresh_offset);
                auto *denominator =
                    reinterpret_cast<std::uint32_t *>(obj + k_mode_desc_refresh_offset + 4);
                *numerator   = 0;
                *denominator = 0;
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
