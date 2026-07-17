#include "games/unity/registry.hpp"

namespace games::unity {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    auto registry() -> UnityRegistry & {
        static UnityRegistry instance;
        return instance;
    }
#pragma clang diagnostic pop
} // namespace games::unity
