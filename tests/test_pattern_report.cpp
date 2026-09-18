#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/validate.hpp"

#include "games/game_data.hpp"

// A miniature stand-in for a real game_data: two scanned patterns, one of which
// a hook treats as optional. Using the real game specializations here would drag
// in the Windows-only hook headers.
namespace {
    struct MockGame {};

    struct MockAddrs {
        std::optional<std::uintptr_t> required_addr;
        std::optional<std::uintptr_t> optional_addr;
        std::optional<std::uintptr_t> unscanned_addr;
    };

    using MockField = std::optional<std::uintptr_t> MockAddrs::*;

    struct MockData {
        using ResolvedAddresses = MockAddrs;

        static constexpr auto scan_entries = std::to_array<games::ScanEntry<MockAddrs>>({
            {.name = "REQUIRED_SIG", .field = &MockAddrs::required_addr, .offset = 0, .bytes = "90"},
            {.name = "OPTIONAL_SIG", .field = &MockAddrs::optional_addr, .offset = 0, .bytes = "91"},
        });
    };

    struct MockHook {};

    // Mirrors RegistryOps::pattern_name. That one lives inside a class template
    // parameterized on a HookList of real hooks, which cannot be instantiated
    // outside a game build, so the lookup is exercised through a copy.
    auto pattern_name(MockField field) -> std::string {
        for (const auto &entry : MockData::scan_entries) {
            if (entry.field == field) {
                return std::string(entry.name);
            }
        }
        return "<unscanned>";
    }
} // namespace

namespace hooks {
    template<>
    struct HookTraits<MockHook> {
        [[maybe_unused]]
        static constexpr std::string_view name = "MockHook";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<MockField, 1> {
            &MockAddrs::required_addr,
        };
        static constexpr auto optional_patterns = std::array<MockField, 1> {
            &MockAddrs::optional_addr,
        };
    };
} // namespace hooks

TEST_CASE("pattern_name resolves a member pointer to its signature name", "[registry][patterns]") {
    CHECK(pattern_name(&MockAddrs::required_addr) == "REQUIRED_SIG");
    CHECK(pattern_name(&MockAddrs::optional_addr) == "OPTIONAL_SIG");
}

// A trait may name a field the game never scans for. Reporting that as a missing
// optional is still useful, so the lookup degrades instead of reading past the end.
TEST_CASE("pattern_name falls back when the field has no scan entry", "[registry][patterns]") {
    CHECK(pattern_name(&MockAddrs::unscanned_addr) == "<unscanned>");
}

TEST_CASE("required and optional patterns are disjoint", "[registry][patterns]") {
    STATIC_CHECK(hooks::patterns_are_disjoint<MockHook>());
}
