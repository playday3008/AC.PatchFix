#pragma once

#include "games/syndicate/hooks/language_unlock.hpp"
#include "hooks/registry/registry.hpp"

namespace games::syndicate {
    using AllHooks = hooks::hook_list<LanguageUnlockHook>;

    using SyndicateRegistry = hooks::Registry<AllHooks>;

    auto registry() -> SyndicateRegistry &;
} // namespace games::syndicate
