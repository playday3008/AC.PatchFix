#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/ac/syndicate/hooks/aspect_ratio_fix.hpp"
#include "games/ac/syndicate/hooks/camera_smoothing.hpp"
#include "games/ac/syndicate/hooks/ds4v2_fix.hpp"
#include "games/ac/syndicate/hooks/fps_unlock.hpp"
#include "games/ac/syndicate/hooks/language_unlock.hpp"
#include "games/ac/syndicate/hooks/platform_specs_fix.hpp"
#include "games/ac/syndicate/hooks/prompt_override.hpp"
#include "games/ac/syndicate/hooks/resolution_fix.hpp"

namespace games::ac::syndicate {
    using AllHooks = hooks::hook_list<PlatformSpecsFixHook,
                                      ResolutionFixHook,
                                      AspectRatioFixHook,
                                      DS4v2FixHook,
                                      PromptOverrideHook,
                                      CameraSmoothingHook,
                                      FPSUnlockHook,
                                      LanguageUnlockHook>;

    using SyndicateRegistry = hooks::Registry<AllHooks>;

    auto registry() -> SyndicateRegistry &;
} // namespace games::ac::syndicate
