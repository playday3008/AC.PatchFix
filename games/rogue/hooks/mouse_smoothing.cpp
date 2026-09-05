#include "games/rogue/hooks/mouse_smoothing.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

#include <algorithm>
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

        // Raw deltas are single-frame counts; anything past this is either a device
        // glitch or our own overshoot, and must not reach the camera.
        constexpr float k_delta_limit = 30000.0F;

        // Below this the pre-emphasis gain (1/factor) explodes, so treat it as off.
        constexpr float k_min_factor = 0.02F;

        std::atomic<bool> g_reset {true};

        // Cancels a first-order lag applied downstream. The engine is assumed to do
        // engine[n] = engine[n-1] + factor * (input[n] - engine[n-1]); feeding it
        // estimate + (raw - estimate) / factor makes engine[n] land on raw[n].
        class AxisFilter {
            float estimate_ {0.0F};

          public:
            void reset(float raw) { estimate_ = raw; }

            auto apply(float raw, float factor) -> std::int32_t {
                const float out     = estimate_ + ((raw - estimate_) / factor);
                const float limited = std::clamp(out, -k_delta_limit, k_delta_limit);
                estimate_ += factor * (limited - estimate_);
                return static_cast<std::int32_t>(std::lround(limited));
            }
        };

        AxisFilter g_axis_x;
        AxisFilter g_axis_y;

        struct MouseSmoothingFunctor {
            [[maybe_unused]] static constexpr std::string_view name = "MouseSmoothing";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto *delta_x = reinterpret_cast<std::int32_t *>(regs.rdi + k_delta_x_offset);
                auto *delta_y = reinterpret_cast<std::int32_t *>(regs.rdi + k_delta_y_offset);

                const auto  raw_x  = static_cast<float>(*delta_x);
                const auto  raw_y  = static_cast<float>(*delta_y);
                const auto &cfg    = games::rogue::registry().config<Tag>();
                const float factor = cfg.factor.get();

                // Menus drive the hardware cursor from this same buffer, so the
                // compensation must only run while the player is actually in game.
                if (!games::rogue::registry().enabled<Tag>() || !cfg.disable.get() ||
                    factor < k_min_factor || factor >= 1.0F ||
                    !is_in_game().load(std::memory_order_relaxed)) {
                    g_axis_x.reset(raw_x);
                    g_axis_y.reset(raw_y);
                    return;
                }

                if (g_reset.exchange(false, std::memory_order_relaxed)) {
                    g_axis_x.reset(raw_x);
                    g_axis_y.reset(raw_y);
                }

                *delta_x = g_axis_x.apply(raw_x, factor);
                *delta_y = g_axis_y.apply(raw_y, factor);
            }
        };
    } // namespace

    void HookTraits<Tag>::on_reload(const Config & /*cfg*/) {
        g_reset.store(true, std::memory_order_relaxed);
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

        log::get()->info("MouseSmoothingHook: installed");
        return true;
    }
} // namespace hooks
