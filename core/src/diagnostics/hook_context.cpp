#include "core/diagnostics/hook_context.hpp"

#include <string_view>

namespace diagnostics {
    namespace {
        thread_local std::string_view tl_hook_name;
    } // namespace

    void set_current_hook_name(std::string_view name) {
        tl_hook_name = name;
    }

    auto current_hook_name() -> std::string_view {
        return tl_hook_name;
    }
} // namespace diagnostics
