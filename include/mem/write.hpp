#pragma once

#include <cstdint>
#include <cstring>

#include <type_traits>

#include <Windows.h>

#include "mem/protect.hpp"

namespace mem {

    inline auto write(uintptr_t addr, const void *data, size_t size) -> bool {
        auto old = detail::protect(addr, size, PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        std::memcpy(reinterpret_cast<void *>(addr), data, size);
        detail::unprotect(addr, size, *old);
        return true;
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    inline auto write(uintptr_t addr, T value) -> bool {
        auto old = detail::protect(addr, sizeof(T), PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        *reinterpret_cast<T *>(addr) = value;
        detail::unprotect(addr, sizeof(T), *old);
        return true;
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    inline auto read(uintptr_t addr) -> T {
        return *reinterpret_cast<const T *>(addr);
    }

    inline auto nop(uintptr_t addr, size_t count) -> bool {
        auto old = detail::protect(addr, count, PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        std::memset(reinterpret_cast<void *>(addr), 0x90, count);
        detail::unprotect(addr, count, *old);
        return true;
    }

    inline auto ret(uintptr_t addr, uint16_t pop = 0) -> bool {
        if (pop == 0) {
            return write<uint8_t>(addr, 0xC3);
        }
        auto old = detail::protect(addr, 3, PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        *reinterpret_cast<uint8_t *>(addr)      = 0xC2;
        *reinterpret_cast<uint16_t *>(addr + 1) = pop;
        detail::unprotect(addr, 3, *old);
        return true;
    }

} // namespace mem
