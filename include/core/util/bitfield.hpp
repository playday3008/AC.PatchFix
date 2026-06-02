#pragma once

#include <cstddef>
#include <cstdint>

#include <type_traits>
#include <utility>

namespace bitfield {
    template<typename E>
    concept counted_enum = std::is_enum_v<E> && requires { E::_count; };

    namespace detail {
        template<std::size_t Bits>
        using uint_for_bits = std::conditional_t<
            (Bits <= 8),
            std::uint8_t,
            std::conditional_t<(Bits <= 16),
                               std::uint16_t,
                               std::conditional_t<(Bits <= 32), std::uint32_t, std::uint64_t>>>;
    } // namespace detail

    template<counted_enum E>
    using mask_t = detail::uint_for_bits<std::to_underlying(E::_count)>;

    template<counted_enum E>
    constexpr auto bit(E l) -> mask_t<E> {
        return static_cast<mask_t<E>>(1) << std::to_underlying(l);
    }

    template<counted_enum E>
    constexpr auto has(mask_t<E> bf, E l) -> bool {
        return (bf & bit(l)) != 0;
    }

    template<counted_enum E>
    consteval auto make(E begin, E end) -> mask_t<E> {
        mask_t<E> mask = 0;
        for (auto i = static_cast<mask_t<E>>(std::to_underlying(begin));
             i < static_cast<mask_t<E>>(std::to_underlying(end));
             ++i) {
            mask |= static_cast<mask_t<E>>(static_cast<mask_t<E>>(1) << i);
        }
        return mask;
    }

    template<counted_enum E>
    consteval auto make_all() -> mask_t<E> {
        return make(static_cast<E>(0), E::_count);
    }
} // namespace bitfield
