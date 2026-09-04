#include "core/diagnostics/crash_handler.hpp"

#include <cstdint>

#include <atomic>

#include <Windows.h>

#include "core/diagnostics/address_registry.hpp"
#include "core/diagnostics/crash_report.hpp"
#include "core/diagnostics/patch_registry.hpp"

namespace diagnostics {
    namespace {
        constexpr unsigned k_max_foreign_faults = 8;

        void *g_veh_handle = nullptr;

        auto NTAPI veh_handler(EXCEPTION_POINTERS *ep) -> LONG {
            auto code = static_cast<std::uint32_t>(ep->ExceptionRecord->ExceptionCode);
            if (!is_hardware_exception(code)) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            if (code == EXCEPTION_STACK_OVERFLOW) {
                return EXCEPTION_CONTINUE_SEARCH;
            }

            auto rip = static_cast<std::uintptr_t>(ep->ContextRecord->Rip);

            if (is_plugin_address(rip)) {
                log_crash_report_lightweight(ep);
                return EXCEPTION_CONTINUE_SEARCH;
            }

            if (patch_registry::find_patch(rip) != nullptr ||
                patch_registry::find_nearby(rip, 64) != nullptr) {
                log_crash_report_lightweight(ep);
                log_patch_attribution(ep);
                return EXCEPTION_CONTINUE_SEARCH;
            }

            // Faults elsewhere in the process are not ours, but they are the only
            // record of a crash the game itself leaves behind. Log a bounded number
            // of them so an unattributed crash still names a module and offset.
            static std::atomic<unsigned> foreign_faults {0};
            if (foreign_faults.fetch_add(1, std::memory_order_relaxed) < k_max_foreign_faults) {
                log_crash_report_lightweight(ep);
            }

            return EXCEPTION_CONTINUE_SEARCH;
        }
    } // namespace

    void install_veh() {
        if (g_veh_handle == nullptr) {
            g_veh_handle = AddVectoredExceptionHandler(0, veh_handler);
        }
    }

    void uninstall_veh() {
        if (g_veh_handle != nullptr) {
            RemoveVectoredExceptionHandler(g_veh_handle);
            g_veh_handle = nullptr;
        }
    }
} // namespace diagnostics
