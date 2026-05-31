#include "games/rogue/hooks/game_state.hpp"

#include <cstdint>

#include <atomic>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

namespace hooks {
    std::atomic<bool> g_is_in_game {false};

    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_unpause_hook;
        mem::MidHook g_pause_hook;
        mem::MidHook g_pause2_hook;
#pragma clang diagnostic pop

        using Tag = games::rogue::GameStateHook;

        struct GameUnpause {
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                g_is_in_game.store(true, std::memory_order_relaxed);
                *reinterpret_cast<std::uint8_t *>(regs.rcx + 0x2C0) = 0;
            }
        };

        struct GamePause {
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                g_is_in_game.store(false, std::memory_order_relaxed);
                *reinterpret_cast<std::uint8_t *>(regs.r8 + 0x2C0) = 1;
            }
        };

        struct GamePause2 {
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                g_is_in_game.store(false, std::memory_order_relaxed);
                *reinterpret_cast<std::uint8_t *>(regs.rdi + 0x2C0) = 1;
            }
        };
    } // namespace

    auto HookTraits<games::rogue::GameStateHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("GameStateHook: installing");
        auto unpause = addrs.game_unpause.value();
        auto pause   = addrs.game_pause.value();
        if (auto h = mem::make_hook<GameUnpause>(unpause, unpause + 7)) {
            g_unpause_hook = std::move(*h);
        } else {
            log::get()->error("GameStateHook: unpause hook failed: {}", h.error());
            return false;
        }
        if (auto h = mem::make_hook<GamePause>(pause, pause + 8)) {
            g_pause_hook = std::move(*h);
        } else {
            log::get()->error("GameStateHook: pause hook failed: {}", h.error());
            return false;
        }
        if (addrs.game_pause2) {
            auto pause2 = addrs.game_pause2.value();
            if (auto h = mem::make_hook<GamePause2>(pause2, pause2 + 7)) {
                g_pause2_hook = std::move(*h);
            } else {
                log::get()->error("GameStateHook: pause2 hook failed: {}", h.error());
                return false;
            }
            log::get()->trace("GameStateHook: pause2 hook at 0x{:X}", pause2);
        }
        log::get()->trace("GameStateHook: installed");
        return true;
    }
} // namespace hooks
