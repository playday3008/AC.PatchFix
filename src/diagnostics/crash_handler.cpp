#include "diagnostics/crash_handler.hpp"

#include <cstdint>

#include <Windows.h>

#include "diagnostics/address_registry.hpp"
#include "diagnostics/crash_report.hpp"

namespace diagnostics {
    namespace {
        void *g_veh_handle = nullptr;

        auto NTAPI veh_handler(EXCEPTION_POINTERS *ep) -> LONG {
            auto code = static_cast<std::uint32_t>(ep->ExceptionRecord->ExceptionCode);
            if (!is_hardware_exception(code)) {
                return EXCEPTION_CONTINUE_SEARCH;
            }

            auto rip = static_cast<std::uintptr_t>(ep->ContextRecord->Rip);
            if (!is_plugin_address(rip)) {
                return EXCEPTION_CONTINUE_SEARCH;
            }

            log_crash_report_lightweight(ep);
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
