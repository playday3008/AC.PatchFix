#pragma once

#include <array>
#include <atomic>
#include <string_view>

#include <mini/ini.h>

#include "games/game_data.hpp"
#include "games/tags.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"

template<typename G>
struct GameStateHook {};

namespace hooks {
    template<typename G>
    extern std::atomic<bool> g_is_in_game;

    template<typename G>
    struct HookTraits<GameStateHook<G>> {
        using Addrs       = typename games::game_data<G>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

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
