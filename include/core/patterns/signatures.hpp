#pragma once

#include <cstddef>
#include <cstdint>

#include <expected>
#include <string>
#include <string_view>

namespace patterns {
    [[nodiscard]] auto
        find_unique(std::string_view name, std::string_view pat_str, std::ptrdiff_t offset)
            -> std::expected<std::uintptr_t, std::string>;
} // namespace patterns
