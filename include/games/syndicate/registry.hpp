#pragma once

#include "games/syndicate/hooks/language_unlock.hpp"
#include "hooks/registry/registry.hpp"

// Definition in src/games/syndicate/hooks/language_unlock.cpp

namespace games::syndicate {
    using AllHooks = hooks::hook_list<LanguageUnlockHook>;

    using SyndicateRegistry = hooks::Registry<AllHooks>;

    extern SyndicateRegistry g_registry;
} // namespace games::syndicate
