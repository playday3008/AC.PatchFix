#pragma once

#include <cstdint>

#include <string_view>

#include <Windows.h>

namespace diagnostics {
    auto exception_code_name(std::uint32_t code) -> std::string_view;
    auto is_hardware_exception(std::uint32_t code) -> bool;

    auto callback_fault_filter(EXCEPTION_POINTERS *ep) -> int;
    auto install_fault_filter(EXCEPTION_POINTERS *ep, std::string_view hook_name) -> int;

    void log_crash_report(EXCEPTION_POINTERS *ep, std::string_view context_name);
    void log_crash_report_lightweight(EXCEPTION_POINTERS *ep);
} // namespace diagnostics
