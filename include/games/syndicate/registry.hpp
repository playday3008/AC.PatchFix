#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/syndicate/hooks/ds4v2_fix.hpp"
#include "games/syndicate/hooks/fps_unlock.hpp"
#include "games/syndicate/hooks/language_unlock.hpp"
#include "games/syndicate/hooks/platform_specs_fix.hpp"
#include "games/syndicate/hooks/prompt_override.hpp"
#include "games/syndicate/hooks/resolution_fix.hpp"

namespace games::syndicate {
    using AllHooks = hooks::hook_list<PlatformSpecsFixHook,
                                      ResolutionFixHook,
                                      DS4v2FixHook,
                                      PromptOverrideHook,
                                      FPSUnlockHook,
                                      LanguageUnlockHook>;

    using SyndicateRegistry = hooks::Registry<AllHooks>;

    auto registry() -> SyndicateRegistry &;
} // namespace games::syndicate
