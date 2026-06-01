#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <span>
#include <string_view>

namespace diagnostics::patch_registry {
    enum class PatchType : std::uint8_t { mid_hook, byte_write };

    struct PatchEntry {
        std::uintptr_t               base {};
        std::size_t                  size {};
        std::array<std::uint8_t, 64> original_bytes {};
        std::uint8_t                 original_size {};
        std::string_view             hook_name;
        PatchType                    type {};
    };

    void register_patch(std::uintptr_t                base,
                        std::size_t                   size,
                        std::span<const std::uint8_t> original,
                        std::string_view              hook_name,
                        PatchType                     type);

    auto find_patch(std::uintptr_t addr) -> const PatchEntry *;

    auto find_nearby(std::uintptr_t addr, std::size_t threshold = 64) -> const PatchEntry *;

    auto all_patches() -> std::span<const PatchEntry>;
} // namespace diagnostics::patch_registry
