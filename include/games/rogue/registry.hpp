#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/rogue/hooks/display_detection.hpp"
#include "games/rogue/hooks/fov_correction.hpp"
#include "games/rogue/hooks/fps_unlock.hpp"
#include "games/rogue/hooks/full_mode_list.hpp"
#include "games/rogue/hooks/game_state.hpp"
#include "games/rogue/hooks/language_unlock.hpp"
#include "games/rogue/hooks/mode_index_guard.hpp"
#include "games/rogue/hooks/mouse_smoothing.hpp"
#include "games/rogue/hooks/viewport_fitting.hpp"
#include "games/rogue/hooks/viewport_scaling.hpp"

namespace games::rogue {
    using AllHooks = hooks::hook_list<GameStateHook,
                                      DisplayDetectionHook,
                                      ViewportFittingHook,
                                      ViewportScalingHook,
                                      FOVCorrectionHook,
                                      FPSUnlockHook,
                                      LanguageUnlockHook,
                                      ModeIndexGuardHook,
                                      FullModeListHook,
                                      MouseSmoothingHook>;

    using RogueRegistry = hooks::Registry<AllHooks>;

    auto registry() -> RogueRegistry &;
} // namespace games::rogue
