#include "games/syndicate/hooks/language_unlock.hpp"

#include <cstdint>

#include "logger.hpp" // IWYU pragma: keep
#include "mem/hook.hpp"
#include "mem/write.hpp"
#include "mem/x64.hpp"

#include "games/syndicate/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::LanguageUnlockHook;

        constexpr uint32_t k_ww_menu     = 0x00487FFEU;
        constexpr uint32_t k_ww_subtitle = 0x005BFFFEU;
        constexpr uint32_t k_ww_audio    = 0x0048262EU;

        constexpr uintptr_t k_bf_write_fallback_offset = 0x121;

        uint32_t *s_bf1_global = nullptr;
        uint32_t *s_bf2_global = nullptr;
        uint32_t *s_bf3_global = nullptr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_bf_hook;
#pragma clang diagnostic pop

        struct LangBitfieldPatch {
            [[maybe_unused]] static void operator()(mem::Registers & /*regs*/) {
                *s_bf1_global = k_ww_menu;
                *s_bf2_global = k_ww_subtitle;
                *s_bf3_global = k_ww_audio;
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate LanguageUnlockHook: installing");

        const auto &cfg        = games::syndicate::g_registry.config<Tag>();
        bool        unlock_all = cfg.unlock_all.get();

        if (!unlock_all) {
            log::get()->info("Syndicate LanguageUnlockHook: UnlockAll=false, skipping");
            return true;
        }

        auto lang_setup    = addrs.lang_setup.value();
        auto lang_bf_write = addrs.lang_bf_write.value_or(lang_setup + k_bf_write_fallback_offset);

        log::get()->trace("Syndicate LanguageUnlockHook: lang_setup=0x{:X} lang_bf_write=0x{:X}",
                          lang_setup,
                          lang_bf_write);

        s_bf1_global = reinterpret_cast<uint32_t *>(mem::x64::read_rel(lang_bf_write + 2));
        s_bf2_global = reinterpret_cast<uint32_t *>(mem::x64::read_rel(lang_bf_write + 8));
        s_bf3_global = reinterpret_cast<uint32_t *>(mem::x64::read_rel(lang_bf_write + 14));

        log::get()->trace("Syndicate LanguageUnlockHook: bf1=0x{:X} bf2=0x{:X} bf3=0x{:X}",
                          reinterpret_cast<uintptr_t>(s_bf1_global),
                          reinterpret_cast<uintptr_t>(s_bf2_global),
                          reinterpret_cast<uintptr_t>(s_bf3_global));

        *s_bf1_global = k_ww_menu;
        *s_bf2_global = k_ww_subtitle;
        *s_bf3_global = k_ww_audio;

        constexpr uintptr_t k_bf_write_size = 18;
        auto                hook_result =
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
