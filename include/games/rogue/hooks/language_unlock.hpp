#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include <mini/ini.h>

#include "games/rogue/game_data.hpp"
#include "games/rogue/language.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"

namespace games::rogue {
    struct LanguageUnlockHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct HookTraits<games::rogue::LanguageUnlockHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

        static constexpr std::string_view name = "LanguageUnlock";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 3> {
            &Addrs::get_game_id,
            &Addrs::lang_bf_write,
            &Addrs::lang_setup,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &Addrs::get_language,
        };

        struct Config : config_base<Config> {
            ini_field<bool>                   unlock_all {"Language", "UnlockAll", false};
            ini_field<games::rogue::Language> ui_language {"Language",
                                                           "UILanguage",
                                                           games::rogue::Language::None};

            static constexpr auto field_ptrs =
                std::tuple {&Config::unlock_all, &Config::ui_language};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
