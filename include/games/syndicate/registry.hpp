#pragma once

#include "games/syndicate/hooks/drift_fix.hpp"
#include "games/syndicate/hooks/ds4v2_fix.hpp"
#include "games/syndicate/hooks/language_unlock.hpp"
#include "games/syndicate/hooks/platform_specs_fix.hpp"
#include "games/syndicate/hooks/prompt_override.hpp"
#include "games/syndicate/hooks/resolution_fix.hpp"
#include "hooks/registry/registry.hpp"

namespace games::syndicate {
    using AllHooks = hooks::hook_list<PlatformSpecsFixHook,
                                      ResolutionFixHook,
                                      DS4v2FixHook,
                                      PromptOverrideHook,
                                      DriftFixHook,
                                      LanguageUnlockHook>;

    using SyndicateRegistry = hooks::Registry<AllHooks>;

    auto registry() -> SyndicateRegistry &;
} // namespace games::syndicate
