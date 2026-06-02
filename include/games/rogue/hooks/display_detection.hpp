#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <mini/ini.h>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"
#include "core/hooks/registry/parsers.hpp"
#include "core/util/bitfield.hpp"

#include "games/rogue/game_data.hpp"

namespace games::rogue {
    enum class MultiMonitor : std::uint8_t {
        Auto        = 0,
        ForceSingle = 1,
        ForceMulti  = 2,
        _count      = 3,
    };
    static_assert(bitfield::counted_enum<MultiMonitor>);

    struct DisplayDetectionHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct default_parser<games::rogue::MultiMonitor> {
        [[maybe_unused]] static auto operator()(const std::string &s)
            -> games::rogue::MultiMonitor {
            constexpr auto table =
                std::to_array<std::pair<std::string_view, games::rogue::MultiMonitor>>({
                    {"Auto", games::rogue::MultiMonitor::Auto},
                    {"Single", games::rogue::MultiMonitor::ForceSingle},
                    {"Multi", games::rogue::MultiMonitor::ForceMulti},
                    {"Triple", games::rogue::MultiMonitor::ForceMulti},
                });
            return detail::parse_enum(s, table, games::rogue::MultiMonitor::Auto);
        }
    };

    template<>
    struct HookTraits<games::rogue::DisplayDetectionHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "DisplayDetection";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::display_flag,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<games::rogue::MultiMonitor> multi_monitor {"Display",
                                                                 "MultiMonitor",
                                                                 games::rogue::MultiMonitor::Auto};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs = std::tuple {&Config::multi_monitor};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
