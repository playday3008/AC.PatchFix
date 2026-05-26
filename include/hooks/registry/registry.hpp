#pragma once

#include <atomic>
#include <tuple>

#include <mini/ini.h>

#include "hooks/registry/all_hooks.hpp"
#include "hooks/registry/hook_traits.hpp"

// IWYU pragma: begin_exports
#include "hooks/language_unlock.hpp"
// IWYU pragma: end_exports

namespace hooks {
    namespace detail {
        template<typename Tag>
        struct HookState {
            typename HookTraits<Tag>::Config config;
            std::atomic<bool>                enabled {true};
            bool                             installed {false};
        };

        template<typename List>
        struct make_storage;

        template<typename... Tags>
        struct make_storage<hook_list<Tags...>> {
            using type = std::tuple<HookState<Tags>...>;
        };
    } // namespace detail

    class Registry {
        using Storage = typename detail::make_storage<AllHooks>::type;
        Storage states_;

      public:
        void install_all(const patterns::ResolvedAddresses &addrs, mINI::INIStructure &ini);
        void reload(mINI::INIStructure &ini);

        template<typename Tag>
        auto enabled() const -> bool {
            return std::get<detail::HookState<Tag>>(states_).enabled.load(
                std::memory_order_relaxed);
        }

        template<typename Tag, typename Self>
        auto config(this Self &&self) -> auto & {
            return std::get<detail::HookState<Tag>>(std::forward<Self>(self).states_).config;
        }

        template<typename Tag>
        void set_enabled(bool val) {
            std::get<detail::HookState<Tag>>(states_).enabled.store(val, std::memory_order_relaxed);
        }

        template<typename Tag>
        void set_installed(bool val) {
            std::get<detail::HookState<Tag>>(states_).installed = val;
        }

        template<typename Tag>
        auto installed() const -> bool {
            return std::get<detail::HookState<Tag>>(states_).installed;
        }
    };

    extern Registry g_registry;

    template<typename Tag>
    auto enabled() -> bool {
        return g_registry.enabled<Tag>();
    }

    template<typename Tag>
    auto config() -> const auto & {
        return g_registry.config<Tag>();
    }
} // namespace hooks
