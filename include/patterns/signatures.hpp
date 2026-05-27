#pragma once

#include <cstddef>
#include <cstdint>

#include <expected>
#include <string>
#include <string_view>

namespace patterns {
    [[nodiscard]] auto
        find_unique(std::string_view name, std::string_view pat_str, ptrdiff_t offset)
            -> std::expected<uintptr_t, std::string>;
} // namespace patterns
