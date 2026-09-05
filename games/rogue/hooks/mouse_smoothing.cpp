#include "games/rogue/hooks/mouse_smoothing.hpp"

#include <cstddef>
#include <cstdint>

#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/rogue/registry.hpp"

namespace hooks {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_mouse_hook;
#pragma clang diagnostic pop

        using Tag = games::rogue::MouseSmoothingHook;

        // The DIMOUSESTATE buffer inside the DirectInput manager: lX and lY of the
        // delta the input update is about to hand to the event consumers. The copy
        // at +0x2216C is made after they run, so patching that one changes nothing.
        constexpr std::ptrdiff_t k_delta_x_offset = 0x22180;
        constexpr std::ptrdiff_t k_delta_y_offset = 0x22184;

        games::rogue::MouseAxisFilter g_axis_x;
        games::rogue::MouseAxisFilter g_axis_y;

        auto factor_active(float factor) -> bool {
            return factor >= games::rogue::k_mouse_min_factor && factor < 1.0F;
        }

        struct MouseSmoothingFunctor {
            [[maybe_unused]] static constexpr std::string_view name = "MouseSmoothing";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto *delta_x = reinterpret_cast<std::int32_t *>(regs.rdi + k_delta_x_offset);
                auto *delta_y = reinterpret_cast<std::int32_t *>(regs.rdi + k_delta_y_offset);

                const auto  raw_x  = static_cast<float>(*delta_x);
                const auto  raw_y  = static_cast<float>(*delta_y);
                const float factor = games::rogue::registry().config<Tag>().factor.get();

                // Menus drive the hardware cursor from this same buffer, so the
                // compensation must only run while the player is actually in game.
                if (!games::rogue::registry().enabled<Tag>() || !factor_active(factor) ||
                    !is_in_game().load(std::memory_order_relaxed)) {
                    g_axis_x.reset(raw_x);
                    g_axis_y.reset(raw_y);
                    return;
                }

                *delta_x = g_axis_x.apply(raw_x, factor);
                *delta_y = g_axis_y.apply(raw_y, factor);
            }
        };
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        const float factor = cfg.factor.get();
        if (factor < games::rogue::k_mouse_min_factor) {
            log::get()->warn("MouseSmoothing: SmoothingFactor {} is below the {} floor, "
                             "compensation off",
                             factor,
                             games::rogue::k_mouse_min_factor);
        } else if (factor >= 1.0F) {
            log::get()->info("MouseSmoothing: SmoothingFactor 1.0, compensation off");
        } else {
            log::get()->trace("MouseSmoothing: SmoothingFactor {}", factor);
        }
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.mouse_state_update.value();
        log::get()->trace("MouseSmoothingHook: installing at 0x{:X}", addr);

        if (auto h = mem::make_hook<MouseSmoothingFunctor>(addr)) {
            g_mouse_hook = std::move(*h);
        } else {
            log::get()->error("MouseSmoothingHook: hook failed: {}", h.error());
            return false;
        }

        on_reload(games::rogue::registry().config<Tag>());

        log::get()->info("MouseSmoothingHook: installed");
        return true;
    }
} // namespace hooks
