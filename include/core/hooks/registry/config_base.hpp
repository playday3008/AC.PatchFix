#pragma once

#include <tuple>

#include <mini/ini.h>

namespace hooks {
    template<typename Derived>
    class config_base {
        friend Derived;
        config_base() = default;

      public:
        void load_all(mINI::INIStructure &ini) {
            std::apply([&](auto... ptrs)
                           -> void { ((static_cast<Derived *>(this)->*ptrs).load_from(ini), ...); },
                       Derived::field_ptrs);
        }
    };

    struct empty_config : config_base<empty_config> {
        static constexpr auto field_ptrs = std::tuple {};
    };
} // namespace hooks
