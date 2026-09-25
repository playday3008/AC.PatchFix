#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/ac/rogue/hooks/camera_lean.hpp"
#include "games/ac/rogue/hooks/display_detection.hpp"
#include "games/ac/rogue/hooks/fov_correction.hpp"
#include "games/ac/rogue/hooks/fps_unlock.hpp"
#include "games/ac/rogue/hooks/full_mode_list.hpp"
#include "games/ac/rogue/hooks/game_state.hpp"
#include "games/ac/rogue/hooks/language_unlock.hpp"
#include "games/ac/rogue/hooks/mode_index_guard.hpp"
#include "games/ac/rogue/hooks/mouse_smoothing.hpp"
#include "games/ac/rogue/hooks/viewport_fitting.hpp"
#include "games/ac/rogue/hooks/viewport_scaling.hpp"

namespace games::ac::rogue {
    using AllHooks = hooks::hook_list<GameStateHook,
                                      DisplayDetectionHook,
                                      ViewportFittingHook,
                                      ViewportScalingHook,
                                      FOVCorrectionHook,
                                      FPSUnlockHook,
                                      LanguageUnlockHook,
                                      ModeIndexGuardHook,
                                      FullModeListHook,
                                      CameraLeanHook,
                                      MouseSmoothingHook>;

    using RogueRegistry = hooks::Registry<AllHooks>;

    auto registry() -> RogueRegistry &;
} // namespace games::ac::rogue
