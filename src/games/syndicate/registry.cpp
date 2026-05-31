#include "games/syndicate/registry.hpp"

namespace games::syndicate {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    auto registry() -> SyndicateRegistry & {
        static SyndicateRegistry instance;
        return instance;
    }
#pragma clang diagnostic pop
} // namespace games::syndicate
