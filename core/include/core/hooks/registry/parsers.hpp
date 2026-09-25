#pragma once

#include <cctype>
#include <cmath>
#include <cstddef>

#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace hooks {
    namespace detail {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        template<typename T>
        auto sv_from_chars(std::string_view sv, T &value) -> std::from_chars_result {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return std::from_chars(sv.data(), sv.data() + sv.size(), value);
        }

        // True when from_chars consumed the whole view, not just a valid prefix.
        inline auto consumed_all(std::string_view sv, const char *end) -> bool {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return end == sv.data() + sv.size();
        }
#pragma clang diagnostic pop

        inline auto trim(std::string_view sv) -> std::string_view {
            const auto is_space = [](char c) -> bool {
                return std::isspace(static_cast<unsigned char>(c)) != 0;
            };
            while (!sv.empty() && is_space(sv.front())) {
                sv.remove_prefix(1);
            }
            while (!sv.empty() && is_space(sv.back())) {
                sv.remove_suffix(1);
            }
            return sv;
        }

        // from_chars reports success on a partial parse and happily yields nan/inf,
        // both of which propagate into the camera and the frame pacer. Require the
        // whole (trimmed) string to be consumed and the result to be finite.
        inline auto parse_finite(std::string_view sv) -> std::optional<float> {
            sv = trim(sv);
            if (sv.empty()) {
                return std::nullopt;
            }
            float val      = 0.0F;
            auto [ptr, ec] = sv_from_chars(sv, val);
            if (ec != std::errc {} || !consumed_all(sv, ptr) || !std::isfinite(val)) {
                return std::nullopt;
            }
            return val;
        }

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
            const std::string_view num = trim(s);
            int                    raw = 0;
            auto [ptr, ec]             = sv_from_chars(num, raw);
            if (ec == std::errc {} && consumed_all(num, ptr)) {
                if constexpr (requires { E::_count; }) {
                    if (raw < 0 || raw >= static_cast<int>(std::to_underlying(E::_count))) {
                        return fallback;
                    }
                }
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
            return detail::parse_finite(s).value_or(0.0F);
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

    struct ratio_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            const std::string_view str(s);
            if (str.empty() || str == "0") {
                return 0.0F;
            }
            auto colon = str.find(':');
            if (colon != std::string_view::npos) {
                const auto w = detail::parse_finite(str.substr(0, colon));
                const auto h = detail::parse_finite(str.substr(colon + 1));
                if (!w || !h || *h <= 0.0F) {
                    return 0.0F;
                }
                return *w / *h;
            }
            return detail::parse_finite(str).value_or(0.0F);
        }
    };

    struct clamped_unit_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            return std::clamp(default_parser<float> {}(s), 0.0F, 1.0F);
        }
    };
} // namespace hooks
