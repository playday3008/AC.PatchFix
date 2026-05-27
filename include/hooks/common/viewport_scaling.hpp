#pragma once

#include <array>
#include <string_view>

#include <mini/ini.h>

#include "games/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/registry/parsers.hpp"

template<typename G>
struct ViewportScalingHook {};

namespace hooks {
    template<typename G>
    struct HookTraits<ViewportScalingHook<G>> {
        using Addrs       = typename games::game_data<G>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

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
            static constexpr auto                 field_ptrs =
                std::tuple {&Config::ui_stretch_h, &Config::ui_stretch_v};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
