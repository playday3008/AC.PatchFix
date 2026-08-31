#include "games/syndicate/hooks/camera_smoothing.hpp"

#include <cstdint>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

#include "games/syndicate/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::CameraSmoothingHook;

        // The game already gates its own camera smoothing behind a flag, then jumps
        // past the lerp when that flag is set. Forcing the branch to jump always
        // makes the camera target apply directly on every frame.
        //
        // Branch identified by @lnx00 in lnx00/game-patches:
        // x64/acs-patches/src/patches/disable_camera_smoothing.rs
        constexpr std::uint8_t k_op_jnz = 0x75;
        constexpr std::uint8_t k_op_jmp = 0xEB;

        std::uintptr_t g_branch = 0;

        void apply_smoothing_patch(bool disable) {
            if (g_branch == 0) {
                return;
            }

            auto opcode = disable ? k_op_jmp : k_op_jnz;
            if (!mem::write<std::uint8_t>(g_branch, opcode)) {
                log::get()->error("Syndicate CameraSmoothingHook: failed to write branch opcode");
                return;
            }

            log::get()->trace("Syndicate CameraSmoothingHook: smoothing {}",
                              disable ? "disabled" : "enabled");
        }
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        apply_smoothing_patch(cfg.disable.get());
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate CameraSmoothingHook: installing");

        auto branch_addr = addrs.camera_smoothing_jnz.value();

        // Only the opcode byte is rewritten, so refuse to touch anything that is
        // not the jnz the signature was written for.
        auto opcode = mem::read<std::uint8_t>(branch_addr);
        if (opcode != k_op_jnz) {
            log::get()->error(
                "Syndicate CameraSmoothingHook: expected jnz at 0x{:X}, found 0x{:02X}",
                branch_addr,
                opcode);
            return false;
        }

        g_branch = branch_addr;
        log::get()->trace("Syndicate CameraSmoothingHook: smoothing branch at 0x{:X}", branch_addr);

        apply_smoothing_patch(games::syndicate::registry().config<Tag>().disable.get());

        log::get()->info("Syndicate CameraSmoothingHook: installed");
        return true;
    }
} // namespace hooks
