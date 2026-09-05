#include <array>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "games/rogue/hooks/mouse_smoothing.hpp"

using games::rogue::k_mouse_delta_limit;
using games::rogue::MouseAxisFilter;

namespace {
    // The downstream lag the filter is built to cancel.
    struct Engine {
        float state {0.0F};

        auto step(float input, float factor) -> float {
            state += factor * (input - state);
            return state;
        }
    };
} // namespace

TEST_CASE("MouseAxisFilter makes a first-order lag land on the raw delta", "[rogue][mouse]") {
    constexpr float factor = 0.1F;
    constexpr std::array<float, 10>
                    raws {0.0F, 12.0F, 40.0F, -7.0F, 0.0F, 0.0F, 3.0F, 3.0F, -25.0F, 0.0F};
    MouseAxisFilter filter;
    Engine          engine;

    for (float raw : raws) {
        const auto  out    = static_cast<float>(filter.apply(raw, factor));
        const float landed = engine.step(out, factor);
        CHECK(landed == Catch::Approx(raw).margin(0.6F));
    }
}

TEST_CASE("MouseAxisFilter factor 1.0 is a pass-through", "[rogue][mouse]") {
    MouseAxisFilter filter;
    CHECK(filter.apply(17.0F, 1.0F) == 17);
    CHECK(filter.apply(-3.0F, 1.0F) == -3);
    CHECK(filter.apply(0.0F, 1.0F) == 0);
}

TEST_CASE("MouseAxisFilter replays motion lost to the output clamp", "[rogue][mouse]") {
    constexpr float factor = 0.02F;
    // Gain is 50x, so anything past 600 counts saturates the 30000 clamp.
    constexpr float flick = 2000.0F;

    MouseAxisFilter filter;
    Engine          engine;

    float      delivered = 0.0F;
    const auto first     = filter.apply(flick, factor);
    CHECK(static_cast<float>(first) == Catch::Approx(k_mouse_delta_limit));
    delivered += engine.step(static_cast<float>(first), factor);

    // Mouse is now still; the carry has to make up the shortfall.
    for (int i = 0; i < 8; ++i) {
        delivered += engine.step(static_cast<float>(filter.apply(0.0F, factor)), factor);
    }

    CHECK(delivered == Catch::Approx(flick).margin(1.0F));
}
