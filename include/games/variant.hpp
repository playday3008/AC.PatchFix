#pragma once

#include <variant>

#include "games/rogue/registry.hpp"
#include "games/syndicate/registry.hpp"

namespace games {
    using RegistryVariant = std::variant<rogue::RogueRegistry *, syndicate::SyndicateRegistry *>;

    template<typename R>
    struct registry_game;

    template<>
    struct registry_game<rogue::RogueRegistry> {
        using type = Rogue;
    };

    template<>
    struct registry_game<syndicate::SyndicateRegistry> {
        using type = Syndicate;
    };

    template<typename R>
    using registry_game_t = typename registry_game<R>::type;
} // namespace games
