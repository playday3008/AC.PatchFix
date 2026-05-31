#pragma once

#include <tuple>

#include <mini/ini.h>

namespace hooks {
    template<typename Derived>
    struct config_base {
        void load_all(mINI::INIStructure &ini) {
            std::apply([&](auto... ptrs)
                           -> void { ((static_cast<Derived *>(this)->*ptrs).load_from(ini), ...); },
                       Derived::field_ptrs);
        }
    };

    struct empty_config {
        static constexpr auto field_ptrs = std::tuple {};

        void load_all(mINI::INIStructure & /*unused*/) {}
    };
} // namespace hooks
