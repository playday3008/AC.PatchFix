#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

#include "games/unity/game_data.hpp"

namespace games::unity {
    struct AimFromPeaksHook {};
} // namespace games::unity

namespace hooks {
    template<>
    struct HookTraits<games::unity::AimFromPeaksHook> {
        using Addrs        = games::game_data<games::Unity>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "AimFromPeaks";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 3> {
            &Addrs::aim_peak_empty_cb,
            &Addrs::aim_peak_working_cb,
            &Addrs::aim_peak_sentinel,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
