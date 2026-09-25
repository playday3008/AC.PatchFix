#pragma once

#include <atomic>
#include <string>
#include <string_view>

#include <mini/ini.h>

#include "core/hooks/registry/parsers.hpp"

namespace hooks {
    template<typename T, typename Parser = default_parser<T>>
    struct ini_field {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::string_view section;
        std::string_view key;
        T                default_value;
        std::atomic<T>   value;
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        constexpr ini_field(std::string_view sec, std::string_view k, T default_val)
            : section(sec),
              key(k),
              default_value(default_val),
              value(default_val) {}

        auto get() const -> T { return value.load(std::memory_order_relaxed); }
        void store(T val) { value.store(val, std::memory_order_relaxed); }

        // Deleting a key means "go back to the default". Leaving the previous value
        // in place instead made the removal a silent no-op until the next launch.
        void load_from(mINI::INIStructure &ini) {
            std::string s(section);
            std::string k(key);
            const T     val = (ini.has(s) && ini[s].has(k)) ? Parser {}(ini[s][k]) : default_value;
            value.store(val, std::memory_order_relaxed);
        }
    };
} // namespace hooks
