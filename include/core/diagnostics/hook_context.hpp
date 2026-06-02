#pragma once

#include <string_view>

namespace diagnostics {
    void set_current_hook_name(std::string_view name);
    auto current_hook_name() -> std::string_view;
} // namespace diagnostics
