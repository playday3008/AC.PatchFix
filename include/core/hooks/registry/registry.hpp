#pragma once

#include <atomic>
#include <tuple>
#include <utility>

#include <mini/ini.h>

#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

namespace hooks {
    namespace detail {
        template<typename Tag>
        struct HookState {
            HookTraits<Tag>::Config config;
            std::atomic<bool>       enabled {true};
            bool                    installed {false};
        };

        template<typename List>
        struct make_storage;

        template<typename... Tags>
        struct make_storage<hook_list<Tags...>> {
            using type = std::tuple<HookState<Tags>...>;
        };
    } // namespace detail

    template<typename HookList>
    class Registry {
        using Storage = detail::make_storage<HookList>::type;
        Storage states_;

      public:
        using hook_list_type = HookList;

        template<typename Addrs>
        void install_all(const Addrs &addrs, mINI::INIStructure &ini);

        void reload(mINI::INIStructure &ini);

        template<typename Tag>
        [[nodiscard]] auto enabled() const -> bool {
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
        [[nodiscard]] auto installed() const -> bool {
            return std::get<detail::HookState<Tag>>(states_).installed;
        }
    };
} // namespace hooks

#include "core/hooks/registry/registry_impl.hpp" // IWYU pragma: keep
