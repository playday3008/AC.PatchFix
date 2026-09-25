#include "games/ac/syndicate/registry.hpp"

namespace games::ac::syndicate {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    auto registry() -> SyndicateRegistry & {
        static SyndicateRegistry instance;
        return instance;
    }
#pragma clang diagnostic pop
} // namespace games::ac::syndicate
