#pragma once

#include <array>
#include <string_view>

#include <mini/ini.h>

#include "config/enums.hpp"
#include "games/game_data.hpp"
#include "hooks/common/viewport_fitting.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"

template<typename G>
struct FOVCorrectionHook {};

template<typename G>
[[nodiscard]] auto compute_hor_plus_correction() -> float;

namespace hooks {
    template<typename G>
    struct HookTraits<FOVCorrectionHook<G>> {
        using Addrs       = typename games::game_data<G>::ResolvedAddresses;
        using PatternField = std::optional<uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FOVCorrection";

        using hard_deps = dep_list<ViewportFittingHook<G>>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::fov_store,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<FovMode>    mode {"FOV", "Mode", FovMode::Auto};
            ini_field<float>      multiplier {"FOV", "Multiplier", 1.0F};
            static constexpr auto field_ptrs = std::tuple {&Config::mode, &Config::multiplier};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
