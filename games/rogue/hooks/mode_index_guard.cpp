#include "games/rogue/hooks/mode_index_guard.hpp"

#include <cstdint>

#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::ModeIndexGuardHook;

        // Display manager -> swapchain owner, then its mode list and entry count.
        constexpr std::uintptr_t k_display_owner = 0xA08;
        constexpr std::uintptr_t k_mode_count    = 0x1B2;

        // Stack layout at the entry of the getter: return address, then the fifth
        // argument (the refresh rate out parameter) in its home slot.
        constexpr std::uintptr_t k_arg5_slot = 0x28;

        constexpr unsigned k_max_reports = 8;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook          g_hook;
        std::atomic<unsigned> g_reports {0};
#pragma clang diagnostic pop

        // The getter indexes its mode array with no bounds check, so the -1 the
        // settings code stores when the saved mode is missing from the list reads
        // 4 GB past the array and faults. Mirror the clamp the mode setter already
        // does on the same index.
        struct ClampModeIndex {
            [[maybe_unused]] static constexpr std::string_view name = "ModeIndexGuard";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                const auto owner = *reinterpret_cast<std::uintptr_t *>(regs.rcx + k_display_owner);
                if (owner == 0) {
                    return;
                }

                const auto count = *reinterpret_cast<std::uint16_t *>(owner + k_mode_count);
                const auto index = static_cast<std::uint32_t>(regs.rdx);
                if (index < count) {
                    return;
                }

                const auto reported = g_reports.fetch_add(1, std::memory_order_relaxed);
                if (reported < k_max_reports) {
                    log::get()->warn("ModeIndexGuard: mode index {} out of range, list holds {}",
                                     static_cast<std::int32_t>(index),
                                     count);
                }

                if (count != 0) {
                    regs.rdx = count - 1U;
                    return;
                }

                // Nothing to clamp to. Zero the out parameters and return, since the
                // caller reads them either way.
                auto *out_refresh = *reinterpret_cast<std::uint32_t **>(regs.rsp + k_arg5_slot);
                *reinterpret_cast<std::uint32_t *>(regs.r8) = 0;
                *reinterpret_cast<std::uint32_t *>(regs.r9) = 0;
                *out_refresh                                = 0;

                regs.rax            = reinterpret_cast<std::uintptr_t>(out_refresh);
                regs.trampoline_rsp = regs.rsp;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.mode_get_by_index.value();

        auto hook_result = mem::make_hook<ClampModeIndex>(addr);
        if (!hook_result) {
            log::get()->error("ModeIndexGuard: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("ModeIndexGuard: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
