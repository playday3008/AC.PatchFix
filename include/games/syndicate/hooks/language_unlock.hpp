#pragma once

#include <array>
#include <string_view>

#include "games/syndicate/game_data.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"

namespace games::syndicate {
    struct LanguageUnlockHook {};
} // namespace games::syndicate

namespace hooks {
    template<>
    struct HookTraits<games::syndicate::LanguageUnlockHook> {
        using Addrs        = games::game_data<games::Syndicate>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

        static constexpr std::string_view name = "LanguageUnlock";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::lang_setup,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &Addrs::lang_bf_write,
        };

        struct Config : config_base<Config> {
            ini_field<bool>       unlock_all {"Language", "UnlockAll", false};
            static constexpr auto field_ptrs = std::tuple {&Config::unlock_all};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
