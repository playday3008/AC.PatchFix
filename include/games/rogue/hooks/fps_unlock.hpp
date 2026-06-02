#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include <mini/ini.h>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"

#include "games/rogue/game_data.hpp"
#include "games/rogue/hooks/game_state.hpp"

namespace games::rogue {
    struct FPSUnlockHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct HookTraits<games::rogue::FPSUnlockHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FPSUnlock";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<games::rogue::GameStateHook>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::fps_sleep_branch,
            &Addrs::fps_frame_time,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<float> target {"FPS", "Target", 0.0F};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs = std::tuple {&Config::target};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
