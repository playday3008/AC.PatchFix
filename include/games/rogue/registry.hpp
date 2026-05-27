#pragma once

#include "games/rogue/game_data.hpp"
#include "games/rogue/hooks/language_unlock.hpp"
#include "hooks/common/display_detection.hpp"
#include "hooks/common/fov_correction.hpp"
#include "hooks/common/fps_unlock.hpp"
#include "hooks/common/game_state.hpp"
#include "hooks/common/viewport_fitting.hpp"
#include "hooks/common/viewport_scaling.hpp"
#include "hooks/registry/registry.hpp"

namespace games::rogue {
    using AllHooks = hooks::hook_list< ::GameStateHook<Rogue>,
                                       ::DisplayDetectionHook<Rogue>,
                                       ::ViewportFittingHook<Rogue>,
                                       ::ViewportScalingHook<Rogue>,
                                       ::FOVCorrectionHook<Rogue>,
                                       ::FPSUnlockHook<Rogue>,
                                       LanguageUnlockHook>;

    using RogueRegistry = hooks::Registry<AllHooks>;

    extern RogueRegistry g_registry;
} // namespace games::rogue
