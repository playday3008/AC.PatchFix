#pragma once

#include <cstddef>
#include <tuple>

#include <mini/ini.h>

namespace hooks {
    template<typename Derived>
    class config_base {
        friend Derived;
        config_base() = default;

      public:
        void load_all(mINI::INIStructure &ini) {
            static_assert(std::tuple_size_v<decltype(Derived::field_ptrs)> == Derived::field_count,
                          "field_ptrs size must match field_count");
            std::apply([&](auto... ptrs)
                           -> void { ((static_cast<Derived *>(this)->*ptrs).load_from(ini), ...); },
                       Derived::field_ptrs);
        }
    };

    struct empty_config : config_base<empty_config> {
        static constexpr std::size_t field_count = 0;
        static constexpr auto        field_ptrs  = std::tuple {};
    };
} // namespace hooks
