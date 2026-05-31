#include "games/syndicate/hooks/drift_fix.hpp"

#include "logger.hpp" // IWYU pragma: keep

namespace hooks {
    auto HookTraits<games::syndicate::DriftFixHook>::install(const Addrs & /*addrs*/) -> bool {
        log::get()->info("Syndicate DriftFixHook: not yet implemented (requires dynamic analysis)");
        return true;
    }
} // namespace hooks
