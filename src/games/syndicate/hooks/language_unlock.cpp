#include "games/syndicate/hooks/language_unlock.hpp"

#include <cstdint>

#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"
#include "mem/x64.hpp"

#include "games/syndicate/language.hpp"
#include "games/syndicate/registry.hpp"

namespace hooks {
    using games::syndicate::k_all_menu;
    using games::syndicate::k_all_subtitle;
    using games::syndicate::k_all_audio;

    namespace {
        using Tag = games::syndicate::LanguageUnlockHook;

        constexpr std::uintptr_t k_bf_write_fallback_offset = 0x121;

        std::uint32_t *s_menu_bf_global     = nullptr;
        std::uint32_t *s_subtitle_bf_global = nullptr;
        std::uint32_t *s_audio_bf_global    = nullptr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_bf_hook;
#pragma clang diagnostic pop

        struct LangBitfieldPatch {
            [[maybe_unused]] static void operator()(mem::Registers & /*regs*/) {
                *s_menu_bf_global     = k_all_menu;
                *s_subtitle_bf_global = k_all_subtitle;
                *s_audio_bf_global    = k_all_audio;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate LanguageUnlockHook: installing");

        const auto &cfg        = games::syndicate::g_registry.config<Tag>();
        const bool  unlock_all = cfg.unlock_all.get();

        if (!unlock_all) {
            log::get()->info("Syndicate LanguageUnlockHook: UnlockAll=false, skipping");
            return true;
        }

        auto lang_setup    = addrs.lang_setup.value();
        auto lang_bf_write = addrs.lang_bf_write.value_or(lang_setup + k_bf_write_fallback_offset);

        log::get()->trace("Syndicate LanguageUnlockHook: lang_setup=0x{:X} lang_bf_write=0x{:X}",
                          lang_setup,
                          lang_bf_write);

        s_menu_bf_global = reinterpret_cast<std::uint32_t *>(mem::x64::read_rel(lang_bf_write + 2));
        s_subtitle_bf_global =
            reinterpret_cast<std::uint32_t *>(mem::x64::read_rel(lang_bf_write + 8));
        s_audio_bf_global =
            reinterpret_cast<std::uint32_t *>(mem::x64::read_rel(lang_bf_write + 14));

        log::get()->trace(
            "Syndicate LanguageUnlockHook: menu_bf=0x{:X} subtitle_bf=0x{:X} audio_bf=0x{:X}",
            reinterpret_cast<std::uintptr_t>(s_menu_bf_global),
            reinterpret_cast<std::uintptr_t>(s_subtitle_bf_global),
            reinterpret_cast<std::uintptr_t>(s_audio_bf_global));

        *s_menu_bf_global     = k_all_menu;
        *s_subtitle_bf_global = k_all_subtitle;
        *s_audio_bf_global    = k_all_audio;

        constexpr std::uintptr_t k_bf_write_size = 18;
        auto                     hook_result =
            mem::make_hook<LangBitfieldPatch>(lang_bf_write, lang_bf_write + k_bf_write_size);
        if (!hook_result) {
            log::get()->error("Syndicate LanguageUnlockHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_bf_hook = std::move(*hook_result);

        log::get()->trace("Syndicate LanguageUnlockHook: installed");
        return true;
    }
} // namespace hooks
