#pragma once

#include "core/hooks/registry/registry.hpp"

namespace games::unity {
    using AllHooks = hooks::hook_list<>;

    using UnityRegistry = hooks::Registry<AllHooks>;

    auto registry() -> UnityRegistry &;
} // namespace games::unity
