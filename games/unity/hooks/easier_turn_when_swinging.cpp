#include "games/unity/hooks/easier_turn_when_swinging.hpp"

#include <cstdint>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

namespace hooks {
    namespace {
        using Tag = games::unity::EasierTurnWhenSwingingHook;

        // The movss loads the swing-turn priority constant via RIP-relative rel32.
        // Vanilla reads 2.0f; the fix repoints it to the adjacent 4.0f (+0x18).
        constexpr std::uint32_t  k_bits_2f     = 0x40000000; // 2.0f
        constexpr std::uint32_t  k_bits_4f     = 0x40800000; // 4.0f
        constexpr std::int32_t   k_const_delta = 0x18;       // 4.0f is +0x18 from 2.0f
        constexpr std::uintptr_t k_movss_len   = 8;          // F3 0F 10 35 <rel32>
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.swing_turn_movss.value();
        log::get()->trace("Unity EasierTurnWhenSwingingHook: installing at 0x{:X}", addr);

        auto rel = mem::read<std::int32_t>(addr + 4);
        auto target =
            addr + k_movss_len + static_cast<std::uintptr_t>(static_cast<std::intptr_t>(rel));

        if (mem::read<std::uint32_t>(target) != k_bits_2f) {
            log::get()->error("Unity EasierTurnWhenSwingingHook: expected 2.0f at 0x{:X}, aborting",
                              target);
            return false;
        }
        if (mem::read<std::uint32_t>(target + k_const_delta) != k_bits_4f) {
            log::get()->error("Unity EasierTurnWhenSwingingHook: expected 4.0f at 0x{:X}, aborting",
                              target + k_const_delta);
            return false;
        }

        if (!mem::write<std::int32_t>(addr + 4, rel + k_const_delta)) {
            log::get()->error(
                "Unity EasierTurnWhenSwingingHook: failed to repoint movss rel32 at 0x{:X}",
                addr + 4);
            return false;
        }

        log::get()->info(
            "Unity EasierTurnWhenSwingingHook: repointed swing-turn priority to 4.0f (0x{:X})",
            target + k_const_delta);
        return true;
    }
} // namespace hooks
