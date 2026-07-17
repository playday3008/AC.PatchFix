#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/game_data.hpp"
#include "games/tags.hpp"

namespace games {
    template<>
    struct game_data<Unity> {
        static constexpr std::string_view name     = "Unity";
        static constexpr std::string_view exe_name = "ACU.exe";

        static constexpr bool             vmprotect         = true;
        static constexpr std::string_view integrity_section = ".UBX0";

        struct ResolvedAddresses {
            std::optional<std::uintptr_t> eject_wait_site;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="EJECT_WAIT", .field=&ResolvedAddresses::eject_wait_site, .offset=0x10, .bytes="C7 81 B8 03 00 00 00 00 80 3E C7 81 E8 03 00 00 ? ? ? ? C3"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Unity>);
} // namespace games
