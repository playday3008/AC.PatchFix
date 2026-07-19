#include "games/unity/hooks/no_more_failed_bomb_throws.hpp"

#include <cstddef>
#include <cstdint>

#include <array>
#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/call.hpp"
#include "core/mem/hook.hpp"

#include "games/unity/registry.hpp"

namespace hooks {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_bomb_exit_hook;
#pragma clang diagnostic pop

        std::atomic<std::uintptr_t> g_onthrow {0};

        using Tag = games::unity::NoMoreFailedBombThrowsHook;

        // void OnThrowBomb(HumanStatesHolder* hs, __m128* vec, char forceNow) — MS x64 ABI.
        using OnThrowBomb = void(std::uintptr_t, void *, char);

        // HumanStatesHolder layout (raw offsets; no typed class layer).
        constexpr std::uintptr_t k_mgr_off       = 0x1CD0; // -> ManagerOfAnimationSignalsReceivers*
        constexpr std::uintptr_t k_arr_off       = 0x40;   // SmallArray.arr ptr
        constexpr std::uintptr_t k_size_off      = 0x4A;   // SmallArray.size (u16)
        constexpr std::uintptr_t k_receiver_size = 0x20;   // signal-receiver element stride
        constexpr std::uintptr_t k_num_listeners = 0x10;   // element.numListenersToThisSignal (u32)
        constexpr std::uint16_t  k_bomb_idx      = 24;     // bomb-throw anim signal slot

        struct BombExitFunctor {
            [[maybe_unused]] static constexpr std::string_view name = "NoMoreFailedBombThrows";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::unity::registry().enabled<Tag>()) {
                    return;
                }
                auto hs = regs.rcx; // shared bomb-exit handler: rcx == humanStates
                if (hs == 0) {
                    return;
                }
                auto manager = *reinterpret_cast<std::uintptr_t *>(hs + k_mgr_off);
                if (manager == 0) {
                    return;
                }
                auto arr  = *reinterpret_cast<std::uintptr_t *>(manager + k_arr_off);
                auto size = *reinterpret_cast<std::uint16_t *>(manager + k_size_off);
                if (arr == 0 || size <= k_bomb_idx) {
                    return;
                }
                auto elem = arr + (static_cast<std::uintptr_t>(k_bomb_idx) * k_receiver_size);
                if (*reinterpret_cast<std::uint32_t *>(elem + k_num_listeners) != 1) {
                    return; // throw anim not mid-flight -> nothing to force
                }
                alignas(16) std::array<std::byte, 16> zero {};
                mem::invoke<OnThrowBomb>(g_onthrow.load(std::memory_order_relaxed),
                                         hs,
                                         zero.data(),
                                         static_cast<char>(1));
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto site = addrs.bomb_exit_site.value();
        g_onthrow.store(addrs.bomb_onthrow_fn.value(), std::memory_order_relaxed);

        log::get()->trace("Unity NoMoreFailedBombThrowsHook: installing at 0x{:X} (onThrow=0x{:X})",
                          site,
                          g_onthrow.load(std::memory_order_relaxed));

        if (auto h = mem::make_hook<BombExitFunctor>(site)) {
            g_bomb_exit_hook = std::move(*h);
        } else {
            log::get()->error("Unity NoMoreFailedBombThrowsHook: hook failed: {}", h.error());
            return false;
        }

        log::get()->info("Unity NoMoreFailedBombThrowsHook: installed at 0x{:X}", site);
        return true;
    }
} // namespace hooks
