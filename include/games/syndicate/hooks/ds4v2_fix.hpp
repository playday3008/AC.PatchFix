#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/syndicate/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"

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
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
