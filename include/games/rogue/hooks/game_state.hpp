#pragma once

#include <cstdint>

#include <array>
#include <atomic>
#include <optional>
#include <string_view>

#include <mini/ini.h>

#include "games/rogue/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"

namespace games::rogue {
    struct GameStateHook {};
} // namespace games::rogue

namespace hooks {
    auto is_in_game() -> std::atomic<bool> &;

    template<>
    struct HookTraits<games::rogue::GameStateHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "GameState";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::game_unpause,
            &Addrs::game_pause,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &Addrs::game_pause2,
        };

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
