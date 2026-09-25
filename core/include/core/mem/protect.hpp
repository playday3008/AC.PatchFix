#pragma once

#include <cstddef>
#include <cstdint>

#include <optional>

#include <Windows.h>

namespace mem {
    enum class ProtectMethod : std::uint8_t { virtual_protect, nt_protect };

    void set_protect_method(ProtectMethod method);

    auto protect_method() -> ProtectMethod;
} // namespace mem

namespace mem::detail {
    auto protect(std::uintptr_t addr, std::size_t size, std::uint32_t new_protect)
        -> std::optional<std::uint32_t>;

    void unprotect(std::uintptr_t addr, std::size_t size, std::uint32_t old_protect);
} // namespace mem::detail
