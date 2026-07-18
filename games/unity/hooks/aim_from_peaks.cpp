#include "games/unity/hooks/aim_from_peaks.hpp"

#include <cstdint>

#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/call.hpp"
#include "core/mem/hook.hpp"

#include "games/unity/registry.hpp"

namespace hooks {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_aim_peak_hook;
#pragma clang diagnostic pop

        std::atomic<std::uintptr_t> g_working_cb {0};
        std::atomic<std::uintptr_t> g_sentinel {0};

        using Tag = games::unity::AimFromPeaksHook;

        // Ledge aim handler: char f(rcx, rdx, r8, r9); writes its result byte to [r9].
        using AimCallback = char(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);

        constexpr std::uintptr_t k_state_handler_off = 0xE0; // [rcx+0xE0] = current state fn ptr

        struct AimFromPeaksFunctor {
            [[maybe_unused]] static constexpr std::string_view name = "AimFromPeaks";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::unity::registry().enabled<Tag>()) {
                    return; // pass-through: original `mov [r9],0; ret` runs unchanged
                }
                auto handler = *reinterpret_cast<std::uintptr_t *>(regs.rcx + k_state_handler_off);
                if (handler != g_sentinel.load(std::memory_order_relaxed)) {
                    return; // slope / V-shape, not a peak -> keep vanilla behavior (no crash)
                }
                // On a peak: run the working (ledge) aim handler with the same args...
                mem::invoke<AimCallback>(g_working_cb.load(std::memory_order_relaxed),
                                         regs.rcx,
                                         regs.rdx,
                                         regs.r8,
                                         regs.r9);
                // ...then redirect the trailing `mov [r9],0` to a throwaway byte so it
                // cannot clobber the result the working handler just wrote to [r9].
                static std::uint8_t scratch = 0;
                regs.r9                     = reinterpret_cast<std::uintptr_t>(&scratch);
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto empty_cb = addrs.aim_peak_empty_cb.value();
        g_working_cb.store(addrs.aim_peak_working_cb.value(), std::memory_order_relaxed);
        g_sentinel.store(addrs.aim_peak_sentinel.value(), std::memory_order_relaxed);

        log::get()->trace(
            "Unity AimFromPeaksHook: installing at 0x{:X} (working=0x{:X}, sentinel=0x{:X})",
            empty_cb,
            g_working_cb.load(std::memory_order_relaxed),
            g_sentinel.load(std::memory_order_relaxed));

        if (auto h = mem::make_hook<AimFromPeaksFunctor>(empty_cb)) {
            g_aim_peak_hook = std::move(*h);
        } else {
            log::get()->error("Unity AimFromPeaksHook: hook failed: {}", h.error());
            return false;
        }

        log::get()->info("Unity AimFromPeaksHook: installed at 0x{:X}", empty_cb);
        return true;
    }
} // namespace hooks
