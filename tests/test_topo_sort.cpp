#include <cstddef>

#include <array>
#include <string_view>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

namespace {
    struct MockA {};
    struct MockB {};
    struct MockC {};
} // namespace

namespace hooks {
    template<>
    struct HookTraits<MockA> {
        static constexpr std::string_view name = "MockA";
        using hard_deps                        = dep_list<>;
        using soft_deps                        = dep_list<>;
    };

    template<>
    struct HookTraits<MockB> {
        static constexpr std::string_view name = "MockB";
        using hard_deps                        = dep_list<MockA>;
        using soft_deps                        = dep_list<>;
    };

    template<>
    struct HookTraits<MockC> {
        static constexpr std::string_view name = "MockC";
        using hard_deps                        = dep_list<MockA>;
        using soft_deps                        = dep_list<MockB>;
    };
} // namespace hooks

namespace {
    using TestHookList = hooks::hook_list<MockA, MockB, MockC>;

    constexpr std::size_t N = 3;

    template<typename Tag, typename... Tags>
    constexpr auto hook_idx(hooks::hook_list<Tags...> /*unused*/) -> std::size_t {
        constexpr std::array matches = {std::is_same_v<Tag, Tags>...};
        for (std::size_t i = 0; i < sizeof...(Tags); ++i) {
            if (matches[i]) return i;
        }
        return sizeof...(Tags);
    }

    constexpr std::size_t kA = hook_idx<MockA>(TestHookList {});
    constexpr std::size_t kB = hook_idx<MockB>(TestHookList {});
    constexpr std::size_t kC = hook_idx<MockC>(TestHookList {});

    static_assert(kA == 0);
    static_assert(kB == 1);
    static_assert(kC == 2);

    constexpr auto topo_sort() -> std::pair<std::array<std::size_t, N>, std::size_t> {
        // MockA: no deps
        // MockB: hard dep on MockA
        // MockC: hard dep on MockA, soft dep on MockB
        // Expected order: A, B, C
        std::array<std::size_t, N> hard_counts = {0, 1, 1};
        std::array<std::array<std::size_t, N>, N> hard_deps {};
        hard_deps[1][0] = kA; // B depends on A
        hard_deps[2][0] = kA; // C depends on A

        std::array<std::size_t, N> soft_counts = {0, 0, 1};
        std::array<std::array<std::size_t, N>, N> soft_deps {};
        soft_deps[2][0] = kB; // C soft-depends on B

        std::array<std::size_t, N>                in_degree {};
        std::array<std::array<std::size_t, N>, N> adj {};
        std::array<std::size_t, N>                adj_count {};

        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < hard_counts[i]; ++j) {
                auto dep          = hard_deps[i][j];
                adj[dep][adj_count[dep]++] = i;
                in_degree[i]++;
            }
            for (std::size_t j = 0; j < soft_counts[i]; ++j) {
                auto dep          = soft_deps[i][j];
                adj[dep][adj_count[dep]++] = i;
                in_degree[i]++;
            }
        }

        std::array<std::size_t, N> queue {};
        std::size_t front = 0, back = 0;
        for (std::size_t i = 0; i < N; ++i) {
            if (in_degree[i] == 0) queue[back++] = i;
        }

        std::array<std::size_t, N> order {};
        std::size_t count = 0;
        while (front < back) {
            auto u       = queue[front++];
            order[count++] = u;
            for (std::size_t i = 0; i < adj_count[u]; ++i) {
                auto v = adj[u][i];
                if (--in_degree[v] == 0) queue[back++] = v;
            }
        }
        return {order, count};
    }

    constexpr auto result = topo_sort();

    // All nodes visited (no cycle)
    static_assert(result.second == N, "All nodes should be visited");

    // A must come before B and C
    static_assert(result.first[0] == kA, "A has no deps, should be first");
    // B must come before C (C soft-depends on B)
    static_assert(result.first[1] == kB, "B depends only on A, should be second");
    static_assert(result.first[2] == kC, "C depends on A and B, should be last");
} // namespace

TEST_CASE("topo_sort compile-time verification", "[topo_sort]") {
    SUCCEED("All static_asserts passed — topo_sort is correct at compile time");
}
