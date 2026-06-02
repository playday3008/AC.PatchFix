#include "games/rogue/hooks/language_unlock.hpp"

#include <cstdint>

#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/call.hpp"
#include "core/mem/hook.hpp"
#include "core/mem/write.hpp"
#include "core/mem/x64.hpp"

#include "games/rogue/language.hpp"
#include "games/rogue/registry.hpp"

namespace hooks {
    using games::rogue::Language;
    using games::rogue::k_all_menu;
    using games::rogue::k_all_audio;

    namespace {
        using Tag = games::rogue::LanguageUnlockHook;

        enum class GameId : std::uint16_t {
            uplay_ww   = 0x37FU,
            steam_ww   = 0x3A6U,
            uplay_ru   = 0x4A2U,
            steam_ru   = 0x4A3U,
            uplay_asia = 0x67DU,
            steam_asia = 0x67EU,
        };

        std::uintptr_t  s_is_steam_addr    = 0;
        std::uint32_t  *s_menu_bf_global   = nullptr;
        std::uint32_t  *s_audio_bf_global  = nullptr;
        std::uint32_t  *s_lang_idx_global  = nullptr;
        std::uint32_t  *s_ui_lang_global   = nullptr;
        Language        s_ui_language      = Language::None;
        GameId          s_real_game_id     = GameId::uplay_ww;
        std::uintptr_t *s_loc_mgr_ptr      = nullptr;
        std::uintptr_t  s_reload_lang_fn   = 0;
        Language        s_current_language = Language::None;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_lang_bf_hook;
        mem::MidHook g_game_id_hook;
#pragma clang diagnostic pop

        struct LangBitfieldPatch {
            [[maybe_unused]] static constexpr std::string_view name = "LanguageUnlock/Bitfield";

            [[maybe_unused]] static void operator()(mem::Registers & /*regs*/) {
                auto orig = *s_menu_bf_global;

                const bool is_steam = mem::invoke<std::uint8_t()>(s_is_steam_addr) != 0;

                if (bitfield::has(orig, Language::Russian)) {
                    s_real_game_id = is_steam ? GameId::steam_ru : GameId::uplay_ru;
                } else if (bitfield::has(orig, Language::Korean) &&
                           bitfield::has(orig, Language::ChineseTrad)) {
                    s_real_game_id = is_steam ? GameId::steam_asia : GameId::uplay_asia;
                } else {
                    s_real_game_id = is_steam ? GameId::steam_ww : GameId::uplay_ww;
                }

                *s_menu_bf_global  = k_all_menu;
                *s_audio_bf_global = k_all_audio;

                if (s_ui_language != Language::None) {
                    if (s_lang_idx_global != nullptr) {
                        *s_lang_idx_global = std::to_underlying(s_ui_language);
                    }
                    if (s_ui_lang_global != nullptr) {
                        *s_ui_lang_global = std::to_underlying(s_ui_language);
                    }
                }
            }
        };

        struct GetGameIdGuard {
            [[maybe_unused]] static constexpr std::string_view name = "LanguageUnlock/GameId";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                regs.rax = std::to_underlying(s_real_game_id);
            }
        };
    } // namespace

    void HookTraits<games::rogue::LanguageUnlockHook>::on_reload(const Config &cfg) {
        auto new_lang = cfg.ui_language.get();
        if (new_lang == s_current_language) {
            return;
        }

        if (new_lang == Language::None) {
            log::get()->warn(
                "LanguageUnlockHook: cannot revert to default language at runtime, restart required");
            return;
        }

        auto idx = std::to_underlying(new_lang);
        if (s_ui_lang_global != nullptr) {
            *s_ui_lang_global = idx;
        }
        if (s_lang_idx_global != nullptr) {
            *s_lang_idx_global = idx;
        }

        if (s_loc_mgr_ptr != nullptr && s_reload_lang_fn != 0) {
            auto loc_mgr = *s_loc_mgr_ptr;
            if (loc_mgr != 0) {
                mem::invoke<void(std::uintptr_t, unsigned, int, int)>(s_reload_lang_fn,
                                                                      loc_mgr,
                                                                      0U,
                                                                      static_cast<int>(idx),
                                                                      0);
                mem::invoke<void(std::uintptr_t, unsigned, int, int)>(s_reload_lang_fn,
                                                                      loc_mgr,
                                                                      2U,
                                                                      static_cast<int>(idx),
                                                                      0);
                log::get()->info("LanguageUnlockHook: reloaded UI strings for language {}", idx);
            }
        }

        s_current_language = new_lang;
        log::get()->info("LanguageUnlockHook: runtime language switched to {}", idx);
    }

    auto HookTraits<games::rogue::LanguageUnlockHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("LanguageUnlockHook: installing");

        const auto &cfg        = games::rogue::registry().config<Tag>();
        const bool  unlock_all = cfg.unlock_all.get();
        s_ui_language          = cfg.ui_language.get();

        if (!unlock_all && s_ui_language == Language::None) {
            log::get()->info("LanguageUnlockHook: nothing enabled, skipping");
            return true;
        }

        if (s_ui_language != Language::None) {
            if (addrs.get_language.has_value()) {
                s_lang_idx_global = reinterpret_cast<std::uint32_t *>(
                    mem::x64::read_rel(addrs.get_language.value() + 6));
                *s_lang_idx_global = std::to_underlying(s_ui_language);
                log::get()->trace("LanguageUnlockHook: lang_idx=0x{:X} override={}",
                                  reinterpret_cast<std::uintptr_t>(s_lang_idx_global),
                                  std::to_underlying(s_ui_language));
            } else {
                log::get()->warn(
                    "LanguageUnlockHook: UILanguage set but GET_LANGUAGE pattern failed");
            }

            // g_uiLanguage is the actual runtime language state read by GetLanguage().
            // g_languageIndex (above) is only copied into g_uiLanguage once during GameInit,
            // so we must write g_uiLanguage directly to survive InitLanguageFromFile's
            // validation pass that reads GetLanguage() after our hook fires.
            auto get_lang_fn = mem::x64::branch_target(addrs.lang_setup.value() + 7);
            if (get_lang_fn) {
                s_ui_lang_global =
                    reinterpret_cast<std::uint32_t *>(mem::x64::read_rel(*get_lang_fn + 2));
                *s_ui_lang_global = std::to_underlying(s_ui_language);
                log::get()->trace("LanguageUnlockHook: ui_lang=0x{:X}",
                                  reinterpret_cast<std::uintptr_t>(s_ui_lang_global));
            } else {
                log::get()->warn(
                    "LanguageUnlockHook: failed to resolve GetLanguage from lang_setup");
            }

            if (addrs.loc_init.has_value()) {
                auto loc_init = addrs.loc_init.value();
                s_loc_mgr_ptr =
                    reinterpret_cast<std::uintptr_t *>(mem::x64::read_rel(loc_init + 0x0C));
                auto reload_opt = mem::x64::branch_target(loc_init + 0x20);
                if (reload_opt) {
                    s_reload_lang_fn = *reload_opt;
                    log::get()->trace("LanguageUnlockHook: loc_mgr_ptr=0x{:X} reload_fn=0x{:X}",
                                      reinterpret_cast<std::uintptr_t>(s_loc_mgr_ptr),
                                      s_reload_lang_fn);
                } else {
                    log::get()->warn(
                        "LanguageUnlockHook: failed to resolve reload_fn from LOC_INIT");
                    s_loc_mgr_ptr = nullptr;
                }
            } else {
                log::get()->info(
                    "LanguageUnlockHook: LOC_INIT pattern not found, runtime switching unavailable");
            }

            s_current_language = s_ui_language;
        }

        if (!unlock_all) {
            log::get()->info("LanguageUnlockHook: UILanguage only (UnlockAll=false)");
            return true;
        }

        auto get_game_id   = addrs.get_game_id.value();
        auto lang_bf_write = addrs.lang_bf_write.value();
        auto lang_setup    = addrs.lang_setup.value();

        auto is_steam_opt = mem::x64::branch_target(get_game_id + 0x12);
        if (!is_steam_opt) {
            log::get()->error("LanguageUnlockHook: failed to resolve IsSteam branch target");
            return false;
        }
        s_is_steam_addr = *is_steam_opt;

        s_menu_bf_global = reinterpret_cast<std::uint32_t *>(mem::x64::read_rel(lang_bf_write + 2));
        s_audio_bf_global =
            reinterpret_cast<std::uint32_t *>(mem::x64::read_rel(lang_bf_write + 8));

        // Pre-patch bitfields before game main calls GetLanguage/SetGameLanguage.
        // The lang file loader may overwrite these later — the callback re-patches.
        *s_menu_bf_global  = k_all_menu;
        *s_audio_bf_global = k_all_audio;

        log::get()->trace("LanguageUnlockHook: IsSteam=0x{:X}", s_is_steam_addr);
        log::get()->trace("LanguageUnlockHook: menu_bf=0x{:X} audio_bf=0x{:X}",
                          reinterpret_cast<std::uintptr_t>(s_menu_bf_global),
                          reinterpret_cast<std::uintptr_t>(s_audio_bf_global));

        if (auto h = mem::make_hook<LangBitfieldPatch>(lang_setup, lang_setup + 7)) {
            g_lang_bf_hook = std::move(*h);
        } else {
            log::get()->error("LanguageUnlockHook: lang_bf hook failed: {}", h.error());
            return false;
        }

        constexpr std::uintptr_t get_game_id_nop_size = 9;
        constexpr std::uintptr_t get_game_id_jmp_size = 5;
        if (!mem::nop(get_game_id, get_game_id_nop_size)) {
            log::get()->error("LanguageUnlockHook: failed to nop get_game_id");
            return false;
        }
        if (auto h = mem::make_hook<GetGameIdGuard>(get_game_id)) {
            g_game_id_hook = std::move(*h);
        } else {
            log::get()->error("LanguageUnlockHook: game_id hook failed: {}", h.error());
            return false;
        }
        if (!mem::ret(get_game_id + get_game_id_jmp_size)) {
            log::get()->error("LanguageUnlockHook: failed to write ret after get_game_id hook");
            return false;
        }

        log::get()->trace("LanguageUnlockHook: installed");
        return true;
    }
} // namespace hooks
