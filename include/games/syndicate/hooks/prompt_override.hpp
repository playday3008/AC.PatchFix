#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "games/syndicate/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/registry/parsers.hpp"

namespace games::syndicate {
    enum class PromptType : std::uint8_t {
        Xbox = 2,
        PS3  = 4,
        PS4  = 5,
    };

    struct PromptOverrideHook {};
} // namespace games::syndicate

namespace hooks {
    template<>
    struct default_parser<games::syndicate::PromptType> {
        [[maybe_unused]] static auto operator()(const std::string &s)
            -> games::syndicate::PromptType {
            constexpr auto table =
                std::to_array<std::pair<std::string_view, games::syndicate::PromptType>>({
                    {"Xbox", games::syndicate::PromptType::Xbox},
                    {"PS3", games::syndicate::PromptType::PS3},
                    {"PS4", games::syndicate::PromptType::PS4},
                });
            return detail::parse_enum(s, table, games::syndicate::PromptType::PS4);
        }
    };

    template<>
    struct HookTraits<games::syndicate::PromptOverrideHook> {
        using Addrs        = games::game_data<games::Syndicate>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "PromptOverride";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::get_active_device_type,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<bool>                         enabled {"PromptOverride", "Enabled", false};
            ini_field<games::syndicate::PromptType> type {"PromptOverride", "Type",
                                                          games::syndicate::PromptType::PS4};

            static constexpr auto field_ptrs = std::tuple {&Config::enabled, &Config::type};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
