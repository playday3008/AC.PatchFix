#pragma once

#include "games/syndicate/hooks/language_unlock.hpp"
#include "games/syndicate/hooks/refresh_rate_fix.hpp"
#include "hooks/registry/registry.hpp"

namespace games::syndicate {
    using AllHooks = hooks::hook_list<LanguageUnlockHook, RefreshRateFixHook>;

    using SyndicateRegistry = hooks::Registry<AllHooks>;

    auto registry() -> SyndicateRegistry &;
} // namespace games::syndicate
