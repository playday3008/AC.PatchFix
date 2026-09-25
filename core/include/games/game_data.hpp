#pragma once

#include <cstddef>
#include <cstdint>

#include <concepts>
#include <optional>
#include <string_view>

namespace games {
    template<typename Addrs>
    struct ScanEntry {
        std::string_view              name;
        std::optional<std::uintptr_t> Addrs::*field;
        std::ptrdiff_t                        offset {};
        std::string_view                      bytes;
    };

    template<typename GameTag>
    struct game_data;

    template<typename G>
    concept ValidGameData = requires {
        { game_data<G>::name } -> std::convertible_to<std::string_view>;
        { game_data<G>::exe_name } -> std::convertible_to<std::string_view>;
        { game_data<G>::scan_entries };
        typename game_data<G>::ResolvedAddresses;
    };

    // A packed game must declare the section-name prefix its packer emits, so
    // the bypass never has to guess which sections belong to the packer. A
    // `vmprotect` declared without it reads as not-packed here, which the
    // static_assert beside the game's own declaration turns into a hard error.
    template<typename G>
    concept HasVmprotect = requires {
        { game_data<G>::vmprotect } -> std::convertible_to<bool>;
        { game_data<G>::vmp_section_prefix } -> std::convertible_to<std::string_view>;
    };

    template<typename G>
    inline constexpr bool game_is_vmprotect = [] -> bool {
        if constexpr (HasVmprotect<G>) {
            return game_data<G>::vmprotect;
        } else {
            return false;
        }
    }();
} // namespace games
