#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <type_traits>

#include <Windows.h>

#include "core/diagnostics/hook_context.hpp"
#include "core/diagnostics/patch_registry.hpp"
#include "core/mem/protect.hpp"

namespace mem {
    inline constexpr std::uint8_t op_nop  = 0x90;
    inline constexpr std::uint8_t op_ret  = 0xC3;
    inline constexpr std::uint8_t op_retn = 0xC2;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"

    [[nodiscard]] inline auto write(std::uintptr_t addr, const void *data, std::size_t size)
        -> bool {
        auto old = detail::protect(addr, size, PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        auto hook_name = diagnostics::current_hook_name();
        if (!hook_name.empty()) {
            const auto *src = reinterpret_cast<const std::uint8_t *>(addr);
            diagnostics::patch_registry::register_patch(
                addr,
                size,
                {src, size},
                hook_name,
                diagnostics::patch_registry::PatchType::byte_write);
        }
        std::memcpy(reinterpret_cast<void *>(addr), data, size);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(addr), size);
        detail::unprotect(addr, size, *old);
        return true;
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] inline auto write(std::uintptr_t addr, T value) -> bool {
        auto old = detail::protect(addr, sizeof(T), PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        auto hook_name = diagnostics::current_hook_name();
        if (!hook_name.empty()) {
            const auto *src = reinterpret_cast<const std::uint8_t *>(addr);
            diagnostics::patch_registry::register_patch(
                addr,
                sizeof(T),
                {src, sizeof(T)},
                hook_name,
                diagnostics::patch_registry::PatchType::byte_write);
        }
        *reinterpret_cast<T *>(addr) = value;
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(addr), sizeof(T));
        detail::unprotect(addr, sizeof(T), *old);
        return true;
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    inline auto read(std::uintptr_t addr) -> T {
        return *reinterpret_cast<const T *>(addr);
    }

    [[nodiscard]] inline auto nop(std::uintptr_t addr, std::size_t count) -> bool {
        auto old = detail::protect(addr, count, PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        auto hook_name = diagnostics::current_hook_name();
        if (!hook_name.empty()) {
            const auto *src = reinterpret_cast<const std::uint8_t *>(addr);
            diagnostics::patch_registry::register_patch(
                addr,
                count,
                {src, count},
                hook_name,
                diagnostics::patch_registry::PatchType::byte_write);
        }
        std::memset(reinterpret_cast<void *>(addr), op_nop, count);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(addr), count);
        detail::unprotect(addr, count, *old);
        return true;
    }

    [[nodiscard]] inline auto ret(std::uintptr_t addr, std::uint16_t pop = 0) -> bool {
        if (pop == 0) {
            return write<std::uint8_t>(addr, op_ret);
        }
        auto old = detail::protect(addr, 3, PAGE_EXECUTE_READWRITE);
        if (!old) {
            return false;
        }
        auto hook_name = diagnostics::current_hook_name();
        if (!hook_name.empty()) {
            const auto *src = reinterpret_cast<const std::uint8_t *>(addr);
            diagnostics::patch_registry::register_patch(
                addr,
                3,
                {src, 3},
                hook_name,
                diagnostics::patch_registry::PatchType::byte_write);
        }
        *reinterpret_cast<std::uint8_t *>(addr)      = op_retn;
        *reinterpret_cast<std::uint16_t *>(addr + 1) = pop;
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(addr), 3);
        detail::unprotect(addr, 3, *old);
        return true;
    }

#pragma clang diagnostic pop
} // namespace mem
