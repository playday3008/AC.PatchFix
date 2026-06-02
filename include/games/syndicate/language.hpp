#pragma once

#include <cstdint>

#include "core/util/bitfield.hpp"

namespace games::syndicate {
    enum class Language : std::uint8_t {
        None        = 0,
        English     = 1,
        French      = 2,
        Spanish     = 3,
        Polish      = 4,
        German      = 5,
        ChineseTrad = 6,
        ChineseSimp = 7,
        Hungarian   = 8,
        Italian     = 9,
        Japanese    = 10,
        Czech       = 11,
        Korean      = 12,
        Russian     = 13,
        Dutch       = 14,
        Danish      = 15,
        Norwegian   = 16,
        Swedish     = 17,
        Portuguese  = 18,
        Brazil      = 19,
        Finnish     = 20,
        Arabic      = 21,
        Mexican     = 22,
        LocTest     = 23,
        _count      = 24,
    };
    static_assert(bitfield::counted_enum<Language>);

    using bf = bitfield::mask_t<Language>;

    // clang-format off

    // All languages that exist across any SKU (PCGamingWiki cross-reference)
    inline constexpr bf k_all_menu =
        bitfield::bit(Language::English)     |
        bitfield::bit(Language::French)      |
        bitfield::bit(Language::Spanish)     |
        bitfield::bit(Language::Polish)      |
        bitfield::bit(Language::German)      |
        bitfield::bit(Language::ChineseTrad) |
        bitfield::bit(Language::ChineseSimp) |
        bitfield::bit(Language::Hungarian)   |
        bitfield::bit(Language::Italian)     |
        bitfield::bit(Language::Japanese)    |
        bitfield::bit(Language::Czech)       |
        bitfield::bit(Language::Korean)      |
        bitfield::bit(Language::Russian)     |
        bitfield::bit(Language::Dutch)       |
        bitfield::bit(Language::Danish)      |
        bitfield::bit(Language::Norwegian)   |
        bitfield::bit(Language::Swedish)     |
        bitfield::bit(Language::Brazil)      |
        bitfield::bit(Language::Finnish)     |
        bitfield::bit(Language::Arabic)      |
        bitfield::bit(Language::Mexican);
    static_assert(k_all_menu == 0x007BFFFEU);

    inline constexpr bf k_all_subtitle = k_all_menu;
    static_assert(k_all_subtitle == 0x007BFFFEU);

    inline constexpr bf k_all_audio =
        bitfield::bit(Language::English)     |
        bitfield::bit(Language::French)      |
        bitfield::bit(Language::Spanish)     |
        bitfield::bit(Language::German)      |
        bitfield::bit(Language::Italian)     |
        bitfield::bit(Language::Japanese)    |
        bitfield::bit(Language::Russian)     |
        bitfield::bit(Language::Brazil)      |
        bitfield::bit(Language::Arabic)      |
        bitfield::bit(Language::Mexican);
    static_assert(k_all_audio == 0x0068262EU);

    // clang-format on

} // namespace games::syndicate
