#include "games/syndicate/hooks/platform_specs_fix.hpp"

#include "logger.hpp" // IWYU pragma: keep

#include "mem/write.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::PlatformSpecsFixHook;
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate PlatformSpecsFixHook: installing");

        auto addr = addrs.specs_enumerate_modes.value();
        if (!mem::ret(addr)) {
            log::get()->error(
                "Syndicate PlatformSpecsFixHook: failed to stub Display_EnumerateAllModes at 0x{:X}",
                addr);
            return false;
        }

        log::get()->info(
            "Syndicate PlatformSpecsFixHook: stubbed Display_EnumerateAllModes at 0x{:X}",
            addr);
        return true;
    }
} // namespace hooks
