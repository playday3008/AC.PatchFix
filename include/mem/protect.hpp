#pragma once

#include <cstdint>

#include <optional>

#include <Windows.h>

namespace mem {

    enum class ProtectMethod { virtual_protect, nt_protect };

    void set_protect_method(ProtectMethod method);

    auto protect_method() -> ProtectMethod;

} // namespace mem

namespace mem::detail {

    auto protect(uintptr_t addr, size_t size, uint32_t new_protect) -> std::optional<uint32_t>;

    void unprotect(uintptr_t addr, size_t size, uint32_t old_protect);

} // namespace mem::detail
