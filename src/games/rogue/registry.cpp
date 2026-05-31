#include "games/rogue/registry.hpp"

namespace games::rogue {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    auto registry() -> RogueRegistry& {
        static RogueRegistry instance;
        return instance;
    }
#pragma clang diagnostic pop
} // namespace games::rogue
