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
    struct ResolutionFixHook {};
} // namespace games::syndicate

namespace hooks {
    template<>
    struct HookTraits<games::syndicate::ResolutionFixHook> {
        using Addrs        = games::game_data<games::Syndicate>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "ResolutionFix";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::mode_insert,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<bool> enabled {"ResolutionFix", "Enabled", true};

            static constexpr auto field_ptrs = std::tuple {&Config::enabled};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
