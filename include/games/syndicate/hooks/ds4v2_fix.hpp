#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "games/syndicate/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"

namespace games::syndicate {
    struct DS4v2FixHook {};
} // namespace games::syndicate

namespace hooks {
    template<>
    struct HookTraits<games::syndicate::DS4v2FixHook> {
        using Addrs        = games::game_data<games::Syndicate>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "DS4v2Fix";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::ds4_type_classify,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 2> {
            &Addrs::di_controller_ctor,
            &Addrs::di_enum_callback,
        };

        struct Config : config_base<Config> {
            ini_field<bool> enabled {"DS4v2Fix", "Enabled", true};

            static constexpr auto field_ptrs = std::tuple {&Config::enabled};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
