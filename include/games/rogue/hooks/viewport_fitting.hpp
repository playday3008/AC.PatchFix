#pragma once

#include <array>
#include <atomic>
#include <string_view>

#include <mini/ini.h>

#include "games/rogue/game_data.hpp"
#include "games/rogue/hooks/game_state.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/registry/parsers.hpp"

namespace games::rogue {
    struct ViewportFittingHook {};
} // namespace games::rogue

namespace hooks {
    extern std::atomic<float> g_current_aspect;

    template<>
    struct HookTraits<games::rogue::ViewportFittingHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

        static constexpr std::string_view name = "ViewportFitting";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<games::rogue::GameStateHook>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::viewport_ratio_load,
            &Addrs::viewport_ratio_mul,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &Addrs::coord_transform,
        };

        struct Config : config_base<Config> {
            ini_field<float, ratio_parser> aspect_ratio {"Display", "AspectRatio", 0.0F};
            static constexpr auto          field_ptrs = std::tuple {&Config::aspect_ratio};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
