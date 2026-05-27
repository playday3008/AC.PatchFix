#pragma once

#include <array>
#include <string_view>

#include <mini/ini.h>

#include "config/enums.hpp"
#include "games/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"

template<typename G>
struct DisplayDetectionHook {};

namespace hooks {
    template<typename G>
    struct HookTraits<DisplayDetectionHook<G>> {
        using Addrs        = typename games::game_data<G>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

        static constexpr std::string_view name = "DisplayDetection";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::display_flag,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<MultiMonitor> multi_monitor {"Display", "MultiMonitor", MultiMonitor::Auto};
            static constexpr auto   field_ptrs = std::tuple {&Config::multi_monitor};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
