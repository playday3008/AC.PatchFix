#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"
#include "core/hooks/registry/parsers.hpp"

#include "games/unity/game_data.hpp"

namespace games::unity {
    struct FreezeFOVHook {};
} // namespace games::unity

namespace hooks {
    template<>
    struct HookTraits<games::unity::FreezeFOVHook> {
        using Addrs        = games::game_data<games::Unity>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FreezeFOV";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::freeze_fov_site,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<float> fov {"FreezeFOV", "Fov", 0.785398F};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::fov};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
