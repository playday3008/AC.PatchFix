#include "core/diagnostics/seh_guard.hpp"

#include <string_view>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/crash_report.hpp"
#include "core/diagnostics/hook_context.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

namespace diagnostics {
    auto guarded_install(bool (*fn)(const void *), const void *addrs, std::string_view hook_name)
        -> bool {
        set_current_hook_name(hook_name);
        __try {
            return fn(addrs);
        } __except (install_fault_filter(GetExceptionInformation(), current_hook_name())) {
            log::get()->critical("Hook installation crashed — hook skipped");
            return false;
        }
    }
} // namespace diagnostics

#pragma clang diagnostic pop
