#pragma once

#include "games/rogue/game_data.hpp"
#include "games/rogue/hooks/language_unlock.hpp"
#include "hooks/common/display_detection.hpp"
#include "hooks/common/fov_correction.hpp"
#include "hooks/common/fps_unlock.hpp"
#include "hooks/common/game_state.hpp"
#include "hooks/common/viewport_fitting.hpp"
#include "hooks/common/viewport_scaling.hpp"
#include "hooks/registry/registry.hpp"

// Definitions in src/games/rogue/hooks/*.cpp
namespace hooks {
    template<>
    auto HookTraits<GameStateHook<games::Rogue>>::install(const Addrs &) -> bool;
    template<>
    auto HookTraits<DisplayDetectionHook<games::Rogue>>::install(const Addrs &) -> bool;
    template<>
    void HookTraits<DisplayDetectionHook<games::Rogue>>::on_reload(const Config &);
    template<>
    auto HookTraits<ViewportFittingHook<games::Rogue>>::install(const Addrs &) -> bool;
    template<>
    void HookTraits<ViewportFittingHook<games::Rogue>>::on_reload(const Config &);
    template<>
    auto HookTraits<ViewportScalingHook<games::Rogue>>::install(const Addrs &) -> bool;
    template<>
    auto HookTraits<FOVCorrectionHook<games::Rogue>>::install(const Addrs &) -> bool;
    template<>
    auto HookTraits<FPSUnlockHook<games::Rogue>>::install(const Addrs &) -> bool;
    template<>
    void HookTraits<FPSUnlockHook<games::Rogue>>::on_reload(const Config &);
} // namespace hooks

namespace games::rogue {
    using AllHooks = hooks::hook_list<::GameStateHook<Rogue>,
                                      ::DisplayDetectionHook<Rogue>,
                                      ::ViewportFittingHook<Rogue>,
                                      ::ViewportScalingHook<Rogue>,
                                      ::FOVCorrectionHook<Rogue>,
                                      ::FPSUnlockHook<Rogue>,
                                      LanguageUnlockHook>;

    using RogueRegistry = hooks::Registry<AllHooks>;

    extern RogueRegistry g_registry;
} // namespace games::rogue
