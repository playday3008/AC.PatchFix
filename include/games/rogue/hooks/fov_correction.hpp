#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <mini/ini.h>

#include "games/rogue/game_data.hpp"
#include "games/rogue/hooks/viewport_fitting.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/registry/parsers.hpp"
#include "util/bitfield.hpp"

namespace games::rogue {
    enum class FovMode : std::uint8_t {
        Auto     = 0,
        VertPlus = 1,
        HorPlus  = 2,
        _count   = 3,
    };
    static_assert(counted_enum<FovMode>);

    [[nodiscard]] auto compute_hor_plus_correction() -> float;

    struct FOVCorrectionHook {};
} // namespace games::rogue

namespace hooks {
    template<>
    struct default_parser<games::rogue::FovMode> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> games::rogue::FovMode {
            constexpr auto table =
                std::to_array<std::pair<std::string_view, games::rogue::FovMode>>({
                    {"Auto", games::rogue::FovMode::Auto},
                    {"VertPlus", games::rogue::FovMode::VertPlus},
                    {"HorPlus", games::rogue::FovMode::HorPlus},
                });
            return detail::parse_enum(s, table, games::rogue::FovMode::Auto);
        }
    };

    template<>
    struct HookTraits<games::rogue::FOVCorrectionHook> {
        using Addrs        = games::game_data<games::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FOVCorrection";

        using hard_deps = dep_list<games::rogue::ViewportFittingHook>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::fov_store,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<games::rogue::FovMode> mode {"FOV", "Mode", games::rogue::FovMode::Auto};
            ini_field<float>                 multiplier {"FOV", "Multiplier", 1.0F};

            static constexpr auto field_ptrs = std::tuple {&Config::mode, &Config::multiplier};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
