#pragma once

#include <concepts>
#include <string_view>

namespace games {
    template<typename GameTag>
    struct game_data;

    template<typename G>
    concept ValidGameData = requires {
        { game_data<G>::name } -> std::convertible_to<std::string_view>;
        { game_data<G>::exe_name } -> std::convertible_to<std::string_view>;
        { game_data<G>::scan_entries };
        typename game_data<G>::ResolvedAddresses;
    };

    template<typename G>
    concept HasVmprotect = requires {
        { game_data<G>::vmprotect } -> std::convertible_to<bool>;
    };

    template<typename G>
    concept HasIntegritySection = requires {
        { game_data<G>::integrity_section } -> std::convertible_to<std::string_view>;
    };

    template<typename G>
    inline constexpr bool game_is_vmprotect = [] -> bool {
        if constexpr (HasVmprotect<G>) {
            return game_data<G>::vmprotect;
        } else {
            return false;
        }
    }();

    template<typename G>
    inline constexpr std::string_view game_integrity_section = [] -> std::string_view {
        if constexpr (HasIntegritySection<G>) {
            return game_data<G>::integrity_section;
        } else {
            return {};
        }
    }();
} // namespace games
