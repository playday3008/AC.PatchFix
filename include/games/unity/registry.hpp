#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/unity/hooks/crouch_fix.hpp"
#include "games/unity/hooks/easier_turn_when_swinging.hpp"
#include "games/unity/hooks/no_wait_unsafe_eject.hpp"

namespace games::unity {
    using AllHooks =
        hooks::hook_list<NoWaitUnsafeEjectHook, CrouchFixHook, EasierTurnWhenSwingingHook>;

    using UnityRegistry = hooks::Registry<AllHooks>;

    auto registry() -> UnityRegistry &;
} // namespace games::unity
