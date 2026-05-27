#include "games/rogue/hooks/language_unlock.hpp"

#include <cstdint>

#include <utility>

#include <injector/assembly.hpp>
#include <injector/calling.hpp>
#include <injector/injector.hpp>

#include "config/language.hpp"
#include "logger.hpp" // IWYU pragma: keep

#include "games/rogue/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::LanguageUnlockHook;

        enum class GameId : uint16_t {
            uplay_ww   = 0x37FU,
            steam_ww   = 0x3A6U,
            uplay_ru   = 0x4A2U,
            steam_ru   = 0x4A3U,
            uplay_asia = 0x67DU,
            steam_asia = 0x67EU,
        };

        uintptr_t s_is_steam_addr      = 0;
        uintptr_t s_set_audio_bf_addr  = 0;
        uint32_t *s_subtitle_bf_global = nullptr;
        uint32_t *s_audio_bf_global    = nullptr;
        uint32_t *s_lang_idx_global    = nullptr;
        Language  s_ui_language        = Language::None;
        GameId    s_real_game_id       = GameId::uplay_ww;

        struct LangBitfieldPatch {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                auto orig = *s_subtitle_bf_global;

                bool is_steam = injector::fastcall<uint8_t()>::call(s_is_steam_addr) != 0;

                if (lang::has(orig, Language::Russian)) {
                    s_real_game_id = is_steam ? GameId::steam_ru : GameId::uplay_ru;
                } else if (lang::has(orig, Language::Korean) &&
                           lang::has(orig, Language::ChineseTrad)) {
                    s_real_game_id = is_steam ? GameId::steam_asia : GameId::uplay_asia;
                } else {
                    s_real_game_id = is_steam ? GameId::steam_ww : GameId::uplay_ww;
                }

                *s_subtitle_bf_global = lang::k_all_languages;
                *s_audio_bf_global    = lang::k_all_languages;

                if (s_lang_idx_global != nullptr && s_ui_language != Language::None) {
                    *s_lang_idx_global = std::to_underlying(s_ui_language);
                }

                regs.rbx = lang::k_all_languages;
                regs.rcx = regs.rbx;
                injector::fastcall<void(uint32_t)>::call(s_set_audio_bf_addr,
                                                         static_cast<uint32_t>(regs.rcx));
            }
        };

        struct GetGameIdGuard {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                regs.rax = std::to_underlying(s_real_game_id);
            }
        };
    } // namespace

    auto HookTraits<games::rogue::LanguageUnlockHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("LanguageUnlockHook: installing");

        const auto &cfg        = games::rogue::g_registry.config<Tag>();
        bool        unlock_all = cfg.unlock_all.get();
        s_ui_language          = cfg.ui_language.get();

        if (!unlock_all && s_ui_language == Language::None) {
            log::get()->info("LanguageUnlockHook: nothing enabled, skipping");
            return true;
        }

        if (s_ui_language != Language::None && addrs.get_language.has_value()) {
            s_lang_idx_global =
                injector::ReadRelativeOffset(addrs.get_language.value() + 6).get<uint32_t>();
            *s_lang_idx_global = std::to_underlying(s_ui_language);
            log::get()->trace("LanguageUnlockHook: lang_idx=0x{:X} override={}",
                              reinterpret_cast<uintptr_t>(s_lang_idx_global),
                              std::to_underlying(s_ui_language));
        } else if (s_ui_language != Language::None) {
            log::get()->warn("LanguageUnlockHook: UILanguage set but GET_LANGUAGE pattern failed");
        }

        if (!unlock_all) {
            log::get()->info("LanguageUnlockHook: UILanguage only (UnlockAll=false)");
            return true;
        }

        auto get_game_id   = addrs.get_game_id.value();
        auto lang_bf_write = addrs.lang_bf_write.value();
        auto lang_setup    = addrs.lang_setup.value();

        s_is_steam_addr     = injector::GetBranchDestination(get_game_id + 0x12).as_int();
        s_set_audio_bf_addr = injector::GetBranchDestination(lang_setup + 0x02).as_int();

        s_subtitle_bf_global = injector::ReadRelativeOffset(lang_bf_write + 2).get<uint32_t>();
        s_audio_bf_global    = injector::ReadRelativeOffset(lang_bf_write + 8).get<uint32_t>();

        // Pre-patch bitfields before game main calls GetLanguage/SetGameLanguage.
        // The lang file loader may overwrite these later — the callback re-patches.
        *s_subtitle_bf_global = lang::k_all_languages;
        *s_audio_bf_global    = lang::k_all_languages;

        log::get()->trace("LanguageUnlockHook: IsSteam=0x{:X} SetAudioBf=0x{:X}",
                          s_is_steam_addr,
                          s_set_audio_bf_addr);
        log::get()->trace("LanguageUnlockHook: subtitle_bf=0x{:X} audio_bf=0x{:X}",
                          reinterpret_cast<uintptr_t>(s_subtitle_bf_global),
                          reinterpret_cast<uintptr_t>(s_audio_bf_global));

        injector::MakeInline<LangBitfieldPatch>(lang_setup, lang_setup + 7);

        constexpr uintptr_t get_game_id_nop_size = 9;
        constexpr uintptr_t get_game_id_jmp_size = 5;
        injector::MakeNOP(get_game_id, get_game_id_nop_size);
        injector::MakeInline<GetGameIdGuard>(get_game_id);
        injector::MakeRET(get_game_id + get_game_id_jmp_size);

        log::get()->trace("LanguageUnlockHook: installed");
        return true;
    }
} // namespace hooks
