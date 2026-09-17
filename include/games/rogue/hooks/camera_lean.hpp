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

namespace games::rogue {
    struct CameraLeanHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct HookTraits<games::rogue::CameraLeanHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "CameraLean";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 3> {
            &Addrs::camera_manager_load,
            &Addrs::track_weight_site,
            &Addrs::camera_interpolate,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            // Frame rate the lean was tuned for. The additive track adds the same
            // increment every frame, so at this rate the fix changes nothing and
            // above it the increment shrinks in proportion. 0 removes the lean.
            ini_field<float> reference_fps {"CameraLean", "ReferenceFPS", 30.0F};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::reference_fps};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
