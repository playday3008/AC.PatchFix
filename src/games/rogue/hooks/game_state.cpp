#include "hooks/common/game_state.hpp"

#include <atomic>

#include <injector/assembly.hpp>
#include <injector/injector.hpp>

#include "logger.hpp" // IWYU pragma: keep

#include "games/rogue/game_data.hpp"

namespace hooks {
    template<>
    std::atomic<bool> g_is_in_game<games::Rogue> {false};

    namespace {
        using G   = games::Rogue;
        using Tag = GameStateHook<G>;

        struct GameUnpause {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                g_is_in_game<G>.store(true, std::memory_order_relaxed);
                *reinterpret_cast<uint8_t *>(regs.rcx + 0x2C0) = 0;
            }
        };

        struct GamePause {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                g_is_in_game<G>.store(false, std::memory_order_relaxed);
                *reinterpret_cast<uint8_t *>(regs.r8 + 0x2C0) = 1;
            }
        };

        struct GamePause2 {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                g_is_in_game<G>.store(false, std::memory_order_relaxed);
                *reinterpret_cast<uint8_t *>(regs.rdi + 0x2C0) = 1;
            }
        };
    } // namespace

    template<>
    auto HookTraits<GameStateHook<games::Rogue>>::install(const Addrs &addrs) -> bool {
        log::get()->trace("GameStateHook: installing");
        auto unpause = addrs.game_unpause.value();
        auto pause   = addrs.game_pause.value();
        injector::MakeInline<GameUnpause>(unpause, unpause + 7);
        injector::MakeInline<GamePause>(pause, pause + 8);
        if (addrs.game_pause2) {
            auto pause2 = addrs.game_pause2.value();
            injector::MakeInline<GamePause2>(pause2, pause2 + 7);
            log::get()->trace("GameStateHook: pause2 hook at 0x{:X}", pause2);
        }
        log::get()->trace("GameStateHook: installed");
        return true;
    }
} // namespace hooks
