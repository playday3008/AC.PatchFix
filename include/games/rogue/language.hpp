#pragma once

#include <cstdint>

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "hooks/registry/parsers.hpp"
#include "util/bitfield.hpp"

namespace games::rogue {
    enum class Language : std::uint8_t {
        None        = 0,
        English     = 1,
        French      = 2,
        Spanish     = 3,
        Polish      = 4,
        German      = 5,
        ChineseTrad = 6,
        Hungarian   = 7,
        Italian     = 8,
        Japanese    = 9,
        Czech       = 10,
        Korean      = 11,
        Russian     = 12,
        Dutch       = 13,
        Danish      = 14,
        Norwegian   = 15,
        Swedish     = 16,
        Portuguese  = 17,
        Brazil      = 18,
        Finnish     = 19,
        Arabic      = 20,
        Mexican     = 21,
        LocTest     = 22,
        _count      = 23,
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
        bitfield::bit(Language::Italian)     |
        bitfield::bit(Language::Japanese)    |
        bitfield::bit(Language::Korean)      |
        bitfield::bit(Language::Russian)     |
        bitfield::bit(Language::Dutch)       |
        bitfield::bit(Language::Brazil)      |
        bitfield::bit(Language::Arabic)      |
        bitfield::bit(Language::Mexican);
    static_assert(k_all_menu == 0x00343B7EU);

    // Rogue shares one bitfield for UI + subtitles
    inline constexpr bf k_all_subtitle = k_all_menu;
    static_assert(k_all_subtitle == 0x00343B7EU);

    inline constexpr bf k_all_audio =
        bitfield::bit(Language::English)     |
        bitfield::bit(Language::French)      |
        bitfield::bit(Language::Spanish)     |
        bitfield::bit(Language::German)      |
        bitfield::bit(Language::Italian)     |
        bitfield::bit(Language::Russian)     |
        bitfield::bit(Language::Brazil);
    static_assert(k_all_audio == 0x0004112EU);

    inline constexpr auto k_language_names = std::to_array<std::pair<std::string_view, Language>>({
        {"None",        Language::None},
        {"English",     Language::English},
        {"French",      Language::French},
        {"Spanish",     Language::Spanish},
        {"Polish",      Language::Polish},
        {"German",      Language::German},
        {"ChineseTrad", Language::ChineseTrad},
        {"Hungarian",   Language::Hungarian},
        {"Italian",     Language::Italian},
        {"Japanese",    Language::Japanese},
        {"Czech",       Language::Czech},
        {"Korean",      Language::Korean},
        {"Russian",     Language::Russian},
        {"Dutch",       Language::Dutch},
        {"Danish",      Language::Danish},
        {"Norwegian",   Language::Norwegian},
        {"Swedish",     Language::Swedish},
        {"Portuguese",  Language::Portuguese},
        {"Brazil",      Language::Brazil},
        {"Finnish",     Language::Finnish},
        {"Arabic",      Language::Arabic},
        {"Mexican",     Language::Mexican},
        {"LocTest",     Language::LocTest},
    });
    // clang-format on
} // namespace games::rogue

namespace hooks {
    template<>
    struct default_parser<games::rogue::Language> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> games::rogue::Language {
            return detail::parse_enum(s,
                                      games::rogue::k_language_names,
                                      games::rogue::Language::None);
        }
    };
} // namespace hooks
