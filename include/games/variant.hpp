#pragma once

#include <variant>

#include "games/rogue/registry.hpp"

namespace games {
    using RegistryVariant = std::variant<
        rogue::RogueRegistry *>;
} // namespace games
