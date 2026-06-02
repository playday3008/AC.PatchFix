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
#include "core/hooks/registry/parsers.hpp"

#include "games/rogue/game_data.hpp"

namespace games::rogue {
    struct ViewportScalingHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct HookTraits<games::rogue::ViewportScalingHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "ViewportScaling";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::scaling_branch_start,
            &Addrs::scaling_branch_end,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &Addrs::scaling_offsets,
        };

        struct Config : config_base<Config> {
            ini_field<float, clamped_unit_parser> ui_stretch_h {"UI", "StretchHorizontal", 0.0F};
            ini_field<float, clamped_unit_parser> ui_stretch_v {"UI", "StretchVertical", 0.0F};

            static constexpr auto field_ptrs =
                std::tuple {&Config::ui_stretch_h, &Config::ui_stretch_v};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
