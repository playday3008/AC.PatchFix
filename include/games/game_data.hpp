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
} // namespace games
