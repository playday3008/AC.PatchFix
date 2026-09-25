#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <span>

#include <Windows.h>

namespace diagnostics {
    inline constexpr std::size_t k_max_frames  = 64;
    inline constexpr std::size_t k_max_sym_len = 256;

    struct StackFrame {
        std::uintptr_t                  address {};
        std::uintptr_t                  module_base {};
        std::uintptr_t                  module_offset {};
        std::array<char, MAX_PATH>      module_name {};
        std::array<char, k_max_sym_len> symbol_name {};
        std::uintptr_t                  symbol_offset {};
        bool                            has_symbol {};
    };

    auto capture_stack(const CONTEXT *ctx, std::span<StackFrame> buf) -> std::span<StackFrame>;
    void resolve_modules(std::span<StackFrame> frames);
    void resolve_symbols(std::span<StackFrame> frames);
} // namespace diagnostics
