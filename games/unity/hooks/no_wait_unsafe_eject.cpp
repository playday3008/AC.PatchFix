#include "games/unity/hooks/no_wait_unsafe_eject.hpp"

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

namespace hooks {
    namespace {
        using Tag = games::unity::NoWaitUnsafeEjectHook;
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.eject_wait_site.value();
        log::get()->trace("Unity NoWaitUnsafeEjectHook: installing at 0x{:X}", addr);

        if (!mem::write<float>(addr, 0.0F)) {
            log::get()->error(
                "Unity NoWaitUnsafeEjectHook: failed to zero eject wait duration at 0x{:X}",
                addr);
            return false;
        }

        log::get()->info("Unity NoWaitUnsafeEjectHook: zeroed eject wait duration at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
