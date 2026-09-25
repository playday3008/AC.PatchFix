#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <mini/ini.h>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"
#include "core/hooks/registry/parsers.hpp"
#include "core/util/bitfield.hpp"

#include "games/ac/rogue/game_data.hpp"
#include "games/ac/rogue/hooks/viewport_fitting.hpp"

namespace games::ac::rogue {
    // NOLINTNEXTLINE(readability-enum-initial-value)
    enum class FovMode : std::uint8_t {
        Auto     = 0,
        VertPlus = 1,
        HorPlus  = 2,
        _count,
    };
    static_assert(bitfield::counted_enum<FovMode>);

    [[nodiscard]] auto compute_hor_plus_correction() -> float;

    struct FOVCorrectionHook {};
} // namespace games::ac::rogue

namespace hooks {
    template<>
    struct default_parser<games::ac::rogue::FovMode> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> games::ac::rogue::FovMode {
            constexpr auto table =
                std::to_array<std::pair<std::string_view, games::ac::rogue::FovMode>>({
                    {"Auto", games::ac::rogue::FovMode::Auto},
                    {"VertPlus", games::ac::rogue::FovMode::VertPlus},
                    {"HorPlus", games::ac::rogue::FovMode::HorPlus},
                });
            return detail::parse_enum(s, table, games::ac::rogue::FovMode::Auto);
        }
    };

    template<>
    struct HookTraits<games::ac::rogue::FOVCorrectionHook> {
        using Addrs        = games::game_data<games::ac::Rogue>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FOVCorrection";

        using hard_deps = dep_list<games::ac::rogue::ViewportFittingHook>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::fov_store,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<games::ac::rogue::FovMode> mode {
                "FOV", "Mode", games::ac::rogue::FovMode::Auto};
            ini_field<float> multiplier {"FOV", "Multiplier", 1.0F};

            static constexpr std::size_t field_count = 2;
            static constexpr auto field_ptrs = std::tuple {&Config::mode, &Config::multiplier};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
