#include "games/unity/hooks/crouch_fix.hpp"

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

namespace hooks {
    namespace {
        using Tag = games::unity::CrouchFixHook;
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.crouch_toggle_site.value();
        log::get()->trace("Unity CrouchFixHook: installing at 0x{:X}", addr);

        if (!mem::nop(addr, 2)) {
            log::get()->error("Unity CrouchFixHook: failed to NOP crouch-toggle jump at 0x{:X}",
                              addr);
            return false;
        }

        log::get()->info("Unity CrouchFixHook: patched crouch-toggle jump at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
