#include "games/syndicate/hooks/resolution_fix.hpp"

#include <cstdint>

#include <numeric>
#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/syndicate/registry.hpp"
#include "games/syndicate/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::ResolutionFixHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        auto is_standard_aspect(std::uint32_t w, std::uint32_t h) -> bool {
            if (w == 0 || h == 0) {
                return true;
            }
            auto g  = std::gcd(w, h);
            auto rw = w / g;
            auto rh = h / g;
            return (rw == 16 && rh == 9) || (rw == 16 && rh == 10) || (rw == 8 && rh == 5) ||
                   (rw == 21 && rh == 9) || (rw == 64 && rh == 27) || (rw == 43 && rh == 18) ||
                   (rw == 32 && rh == 9) || (rw == 5 && rh == 4) || (rw == 4 && rh == 3) ||
                   (rw == 3 && rh == 2);
        }

        struct FilterModeInsert {
            static constexpr std::string_view name = "ResolutionFix";
            [[maybe_unused]] static void      operator()(mem::Registers &regs) {
                const auto *entry = reinterpret_cast<const games::syndicate::ModeEntry *>(regs.rdx);

                if (!is_standard_aspect(entry->width, entry->height)) {
                    log::get()->trace("ResolutionFixHook: filtered {}x{}",
                                      entry->width,
                                      entry->height);
                    auto ret_addr       = *reinterpret_cast<std::uintptr_t *>(regs.rsp);
                    regs.rip            = ret_addr;
                    regs.trampoline_rsp = regs.rsp + 8;
                }
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate ResolutionFixHook: installing");

        const auto &cfg = games::syndicate::registry().config<Tag>();
        if (!cfg.enabled.get()) {
            log::get()->info("Syndicate ResolutionFixHook: disabled, skipping");
            return true;
        }

        auto addr = addrs.mode_insert.value();
        log::get()->trace("Syndicate ResolutionFixHook: ModeList_InsertSorted at 0x{:X}", addr);

        auto hook_result = mem::make_hook<FilterModeInsert>(addr);
        if (!hook_result) {
            log::get()->error("Syndicate ResolutionFixHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("Syndicate ResolutionFixHook: installed");
        return true;
    }
} // namespace hooks
