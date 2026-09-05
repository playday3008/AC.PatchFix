#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"
#include "core/hooks/registry/parsers.hpp"

#include "games/rogue/game_data.hpp"
#include "games/rogue/hooks/game_state.hpp"

namespace games::rogue {
    struct MouseSmoothingHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct HookTraits<games::rogue::MouseSmoothingHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "MouseSmoothing";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<games::rogue::GameStateHook>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::mouse_state_update,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<bool>                       disable {"Input", "DisableMouseSmoothing", true};
            ini_field<float, clamped_unit_parser> factor {"Input", "SmoothingFactor", 0.1F};

            static constexpr std::size_t field_count = 2;
            static constexpr auto field_ptrs = std::tuple {&Config::disable, &Config::factor};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
