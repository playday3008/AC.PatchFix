#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/unity/hooks/no_wait_unsafe_eject.hpp"

namespace games::unity {
    using AllHooks = hooks::hook_list<NoWaitUnsafeEjectHook>;

    using UnityRegistry = hooks::Registry<AllHooks>;

    auto registry() -> UnityRegistry &;
} // namespace games::unity
