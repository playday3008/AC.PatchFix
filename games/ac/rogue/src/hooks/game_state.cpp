#include "games/ac/rogue/hooks/game_state.hpp"

#include <cstdint>

#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/mem/write.hpp"
#include "core/mem/x64.hpp"

#include "games/ac/rogue/structs.hpp"

namespace hooks {
    auto is_in_game() -> std::atomic<bool> & {
        static std::atomic<bool> instance {false};
        return instance;
    }

    auto game_state_ptr() -> std::atomic<games::ac::rogue::GameState *> & {
        static std::atomic<games::ac::rogue::GameState *> instance {nullptr};
        return instance;
    }

    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_unpause_hook;
        mem::MidHook g_pause_hook;
        mem::MidHook g_pause2_hook;
#pragma clang diagnostic pop

        using Tag = games::ac::rogue::GameStateHook;

        std::uintptr_t g_global_var_addr = 0;

        // Each hook replaces a `mov byte [<base>+2C0h], imm` through a different
        // base register, so the flag has to be written through that same register.
        // Going via the g_pGameState global instead would drop the write entirely
        // during the window where the global is still null, and the original
        // instruction is gone, so nothing else would set it.
        void write_pause_flag(std::uintptr_t base, std::uint8_t value, std::string_view site) {
            if (base == 0) {
                log::get()->warn("GameStateHook: {} with a null state pointer", site);
                return;
            }

            auto *state       = reinterpret_cast<games::ac::rogue::GameState *>(base);
            state->pause_flag = value;

            // The engine hands us a live instance here, so adopt it as the cached
            // pointer the other hooks read.
            if (game_state_ptr().load(std::memory_order_relaxed) == nullptr) {
                game_state_ptr().store(state, std::memory_order_relaxed);
                log::get()->info("GameStateHook: adopted GameState* at 0x{:X} from {}", base, site);
            }

            log::get()->trace("GameStateHook: {} (pause_mode={}, is_ready={}, state_index={})",
                              site,
                              state->pause_mode,
                              state->is_ready,
                              state->state_index);
        }

        struct GameUnpause {
            [[maybe_unused]] static constexpr std::string_view name = "GameState/Unpause";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                is_in_game().store(true, std::memory_order_relaxed);
                write_pause_flag(regs.rcx, 0, "unpause");
            }
        };

        struct GamePause {
            [[maybe_unused]] static constexpr std::string_view name = "GameState/Pause";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                is_in_game().store(false, std::memory_order_relaxed);
                write_pause_flag(regs.r8, 1, "pause");
            }
        };

        struct GamePause2 {
            [[maybe_unused]] static constexpr std::string_view name = "GameState/Pause2";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                is_in_game().store(false, std::memory_order_relaxed);
                write_pause_flag(regs.rdi, 1, "pause2");
            }
        };
    } // namespace

    auto HookTraits<games::ac::rogue::GameStateHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("GameStateHook: installing");

        auto pattern_addr = addrs.game_state_global.value();
        g_global_var_addr = mem::x64::read_rel(pattern_addr + 3);
        auto gs_ptr       = mem::read<std::uintptr_t>(g_global_var_addr);

        log::get()->trace("GameStateHook: g_pGameState global at 0x{:X}, instance at 0x{:X}",
                          g_global_var_addr,
                          gs_ptr);

        if (gs_ptr != 0) {
            auto *state = reinterpret_cast<games::ac::rogue::GameState *>(gs_ptr);
            game_state_ptr().store(state, std::memory_order_relaxed);
            log::get()->trace("GameStateHook: cached GameState* (is_ready={}, pause_mode={}, "
                              "state_index={})",
                              state->is_ready,
                              state->pause_mode,
                              state->state_index);
        } else {
            log::get()->warn("GameStateHook: g_pGameState null at install, will late-resolve");
        }

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
