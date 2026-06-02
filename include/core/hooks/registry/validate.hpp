#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

#include <mini/ini.h>

#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

namespace hooks {
    template<typename Tag>
    concept ValidHookTraits = requires(mINI::INIStructure &ini) {
        { HookTraits<Tag>::name } -> std::convertible_to<std::string_view>;
        typename HookTraits<Tag>::hard_deps;
        typename HookTraits<Tag>::soft_deps;
        { HookTraits<Tag>::required_patterns };
        typename HookTraits<Tag>::Config;
        { std::declval<typename HookTraits<Tag>::Config &>().load_all(ini) } -> std::same_as<void>;
    };

    template<typename Tag, typename List>
    constexpr bool is_in_list = false;

    template<typename Tag, typename... Ts>
    constexpr bool is_in_list<Tag, hook_list<Ts...>> = (std::is_same_v<Tag, Ts> || ...);

    template<typename Tag, typename... Ts>
    constexpr bool is_in_list<Tag, dep_list<Ts...>> = (std::is_same_v<Tag, Ts> || ...);

    template<typename Tag, typename All>
    struct validate_hook_deps {
        template<typename... Deps>
        static constexpr auto check_all(dep_list<Deps...> /*unused*/) -> bool {
            if constexpr (sizeof...(Deps) == 0) {
                return true;
            } else {
                return (is_in_list<Deps, All> && ...);
            }
        }

        static_assert(ValidHookTraits<Tag>,
                      "HookTraits<Tag> does not satisfy the required interface");
        static_assert(check_all(typename HookTraits<Tag>::hard_deps {}),
                      "Hard dependency references a hook not in AllHooks");
        static_assert(check_all(typename HookTraits<Tag>::soft_deps {}),
                      "Soft dependency references a hook not in AllHooks");
        static_assert(!is_in_list<Tag, typename HookTraits<Tag>::hard_deps>,
                      "Hook cannot hard-depend on itself");
        static_assert(!is_in_list<Tag, typename HookTraits<Tag>::soft_deps>,
                      "Hook cannot soft-depend on itself");
    };
} // namespace hooks
