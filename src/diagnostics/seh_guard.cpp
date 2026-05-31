#include "diagnostics/seh_guard.hpp"

#include <string_view>

#include <Windows.h>

#include "diagnostics/crash_report.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

namespace diagnostics {
    namespace {
        thread_local std::string_view tl_hook_name;
    } // namespace

    auto guarded_install(bool (*fn)(const void *),
                         const void *addrs,
                         std::string_view hook_name) -> bool {
        tl_hook_name = hook_name;
        __try {
            return fn(addrs);
        } __except (install_fault_filter(GetExceptionInformation(), tl_hook_name)) {
            return false;
        }
    }
} // namespace diagnostics

#pragma clang diagnostic pop
