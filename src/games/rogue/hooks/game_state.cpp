#include "games/rogue/hooks/game_state.hpp"

#include <cstdint>

#include <atomic>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"
#include "mem/write.hpp"
#include "mem/x64.hpp"

#include "games/rogue/structs.hpp"

namespace hooks {
    auto is_in_game() -> std::atomic<bool> & {
        static std::atomic<bool> instance {false};
        return instance;
    }

    auto game_state_ptr() -> std::atomic<games::rogue::GameState *> & {
        static std::atomic<games::rogue::GameState *> instance {nullptr};
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

        using Tag = games::rogue::GameStateHook;

        std::uintptr_t g_global_var_addr = 0;

        auto resolve_state() -> games::rogue::GameState * {
            auto *cached = game_state_ptr().load(std::memory_order_relaxed);
            if (cached != nullptr) {
                return cached;
            }
            if (g_global_var_addr == 0) {
                return nullptr;
            }
            auto raw = mem::read<std::uintptr_t>(g_global_var_addr);
            if (raw == 0) {
                return nullptr;
            }
            auto *state = reinterpret_cast<games::rogue::GameState *>(raw);
            game_state_ptr().store(state, std::memory_order_relaxed);
            log::get()->info("GameStateHook: late-resolved GameState* at 0x{:X}", raw);
            return state;
        }

        struct GameUnpause {
            [[maybe_unused]] static constexpr std::string_view name = "GameState/Unpause";

            [[maybe_unused]] static void operator()(mem::Registers & /*unused*/) {
                is_in_game().store(true, std::memory_order_relaxed);
                auto *state = resolve_state();
                if (state != nullptr) {
                    state->pause_flag = 0;
                    log::get()->trace("GameStateHook: unpause (pause_mode={}, is_ready={}, "
                                      "state_index={})",
                                      state->pause_mode,
                                      state->is_ready,
                                      state->state_index);
                }
            }
        };

        struct GamePause {
            [[maybe_unused]] static constexpr std::string_view name = "GameState/Pause";

            [[maybe_unused]] static void operator()(mem::Registers & /*unused*/) {
                is_in_game().store(false, std::memory_order_relaxed);
                auto *state = resolve_state();
                if (state != nullptr) {
                    state->pause_flag = 1;
                    log::get()->trace("GameStateHook: pause (pause_mode={}, is_ready={}, "
                                      "state_index={})",
                                      state->pause_mode,
                                      state->is_ready,
                                      state->state_index);
                }
            }
        };

        struct GamePause2 {
            [[maybe_unused]] static constexpr std::string_view name = "GameState/Pause2";

            [[maybe_unused]] static void operator()(mem::Registers & /*unused*/) {
                is_in_game().store(false, std::memory_order_relaxed);
                auto *state = resolve_state();
                if (state != nullptr) {
                    state->pause_flag = 1;
                    log::get()->trace("GameStateHook: pause2 (pause_mode={}, is_ready={}, "
                                      "state_index={})",
                                      state->pause_mode,
                                      state->is_ready,
                                      state->state_index);
                }
            }
        };
    } // namespace

    auto HookTraits<games::rogue::GameStateHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("GameStateHook: installing");

        auto pattern_addr = addrs.game_state_global.value();
        g_global_var_addr = mem::x64::read_rel(pattern_addr + 3);
        auto gs_ptr       = mem::read<std::uintptr_t>(g_global_var_addr);

        log::get()->trace("GameStateHook: g_pGameState global at 0x{:X}, instance at 0x{:X}",
                          g_global_var_addr,
                          gs_ptr);

        if (gs_ptr != 0) {
            auto *state = reinterpret_cast<games::rogue::GameState *>(gs_ptr);
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
