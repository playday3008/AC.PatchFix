#include "core/diagnostics/crash_handler.hpp"

#include <cstdint>

#include <Windows.h>

#include "core/diagnostics/address_registry.hpp"
#include "core/diagnostics/crash_report.hpp"
#include "core/diagnostics/patch_registry.hpp"

namespace diagnostics {
    namespace {
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
