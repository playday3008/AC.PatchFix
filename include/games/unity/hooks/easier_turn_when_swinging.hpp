#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

#include "games/unity/game_data.hpp"

namespace games::unity {
    struct EasierTurnWhenSwingingHook {};
} // namespace games::unity

namespace hooks {
    template<>
    struct HookTraits<games::unity::EasierTurnWhenSwingingHook> {
        using Addrs        = games::game_data<games::Unity>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "EasierTurnWhenSwinging";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::swing_turn_movss,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
