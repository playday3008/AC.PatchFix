#pragma once

#include <cctype>
#include <cstddef>

#include <algorithm>
#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "config/enums.hpp"

namespace hooks {
    namespace detail {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        template<typename T>
        auto sv_from_chars(std::string_view sv, T &value) -> std::from_chars_result {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return std::from_chars(sv.data(), sv.data() + sv.size(), value);
        }
#pragma clang diagnostic pop

        inline auto ascii_iequal(std::string_view a, std::string_view b) -> bool {
            return std::ranges::equal(a, b, [](char x, char y) -> bool {
                return std::tolower(static_cast<unsigned char>(x)) ==
                       std::tolower(static_cast<unsigned char>(y));
            });
        }

        template<typename E, std::size_t N>
        auto parse_enum(const std::string                                   &s,
                        const std::array<std::pair<std::string_view, E>, N> &table,
                        E                                                    fallback) -> E {
            for (const auto &[name, val] : table) {
                if (ascii_iequal(s, name)) {
                    return val;
                }
            }
            int raw        = 0;
            auto [ptr, ec] = sv_from_chars(s, raw);
            if (ec == std::errc {}) {
                return static_cast<E>(raw);
            }
            return fallback;
        }
    } // namespace detail

    template<typename T>
    struct default_parser;

    template<>
    struct default_parser<float> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            float val = 0.0F;
            detail::sv_from_chars(s, val);
            return val;
        }
    };

    template<>
    struct default_parser<bool> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> bool {
            constexpr auto truthy = std::to_array<std::string_view>({
                "true",
                "yes",
                "on",
            });
            constexpr auto falsy  = std::to_array<std::string_view>({
                "false",
                "no",
                "off",
            });

            if (std::ranges::any_of(truthy, [&](std::string_view t) -> bool {
                    return detail::ascii_iequal(s, t);
                })) {
                return true;
            }
            if (std::ranges::any_of(falsy, [&](std::string_view f) -> bool {
                    return detail::ascii_iequal(s, f);
                })) {
                return false;
            }
            int val = 0;
            detail::sv_from_chars(s, val);
            return val != 0;
        }
    };

    template<>
    struct default_parser<FovMode> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> FovMode {
            constexpr auto table = std::to_array<std::pair<std::string_view, FovMode>>({
                {"Auto", FovMode::Auto},
                {"VertPlus", FovMode::VertPlus},
                {"HorPlus", FovMode::HorPlus},
            });
            return detail::parse_enum(s, table, FovMode::Auto);
        }
    };

    template<>
    struct default_parser<MultiMonitor> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> MultiMonitor {
            constexpr auto table = std::to_array<std::pair<std::string_view, MultiMonitor>>({
                {"Auto", MultiMonitor::Auto},
                {"Single", MultiMonitor::ForceSingle},
                {"Multi", MultiMonitor::ForceMulti},
                {"Triple", MultiMonitor::ForceMulti},
            });
            return detail::parse_enum(s, table, MultiMonitor::Auto);
        }
    };

    struct ratio_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            std::string_view str(s);
            if (str.empty() || str == "0") {
                return 0.0F;
            }
            auto colon = str.find(':');
            if (colon != std::string_view::npos) {
                float w = 0.0F;
                float h = 0.0F;
                detail::sv_from_chars(str.substr(0, colon), w);
                detail::sv_from_chars(str.substr(colon + 1), h);
                return (h > 0.0F) ? w / h : 0.0F;
            }
            float val = 0.0F;
            detail::sv_from_chars(str, val);
            return val;
        }
    };

    struct clamped_unit_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            return std::clamp(default_parser<float> {}(s), 0.0F, 1.0F);
        }
    };
} // namespace hooks
