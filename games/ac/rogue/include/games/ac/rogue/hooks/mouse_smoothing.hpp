#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"
#include "core/hooks/registry/parsers.hpp"

#include "games/ac/rogue/game_data.hpp"
#include "games/ac/rogue/hooks/game_state.hpp"

namespace games::ac::rogue {
    struct MouseSmoothingHook {};

    // Raw deltas are single-frame counts; anything past this is either a device
    // glitch or our own overshoot, and must not reach the camera.
    inline constexpr float k_mouse_delta_limit = 30000.0F;

    // Below this the pre-emphasis gain (1/factor) explodes, so treat it as off.
    inline constexpr float k_mouse_min_factor = 0.02F;

    // Cancels a first-order lag applied downstream. The engine is assumed to do
    // engine[n] = engine[n-1] + factor * (input[n] - engine[n-1]); feeding it
    // estimate + (raw - estimate) / factor makes engine[n] land on raw[n].
    //
    // The factor is a per-frame coefficient. If the engine's blend turns out to
    // scale with frame time, the value that cancels it changes with frame rate.
    //
    // When the output has to be clamped the engine lands short of raw; the
    // shortfall is carried into the next frame so the integrated motion still
    // matches what the mouse produced.
    class MouseAxisFilter {
        float estimate_ {0.0F};
        float carry_ {0.0F};

      public:
        void reset(float raw) {
            estimate_ = raw;
            carry_    = 0.0F;
        }

        [[nodiscard]] auto apply(float raw, float factor) -> std::int32_t {
            const float target  = raw + carry_;
            const float out     = estimate_ + ((target - estimate_) / factor);
            const float limited = std::clamp(out, -k_mouse_delta_limit, k_mouse_delta_limit);
            estimate_ += factor * (limited - estimate_);
            carry_ = std::clamp(target - estimate_, -k_mouse_delta_limit, k_mouse_delta_limit);
            return static_cast<std::int32_t>(std::lround(limited));
        }
    };
} // namespace games::ac::rogue

namespace hooks {
    template<>
    struct HookTraits<games::ac::rogue::MouseSmoothingHook> {
        using Addrs        = games::game_data<games::ac::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "MouseSmoothing";

        // The functor is a no-op until is_in_game() reports true, which only
        // GameStateHook ever sets, so without it this hook does nothing.
        using hard_deps = dep_list<games::ac::rogue::GameStateHook>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::mouse_state_update,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<float, clamped_unit_parser> factor {"Input", "SmoothingFactor", 0.1F};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::factor};
        };

        static void on_reload(const Config &cfg);
        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
