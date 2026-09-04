#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

#include "games/rogue/game_data.hpp"

namespace games::rogue {
    struct FullModeListHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct HookTraits<games::rogue::FullModeListHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FullModeList";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::mode_list_build,
            &Addrs::mode_list_reserve_site,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
