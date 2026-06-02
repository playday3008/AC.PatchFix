#pragma once

#include <cstddef>

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <mini/ini.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/crash_journal.hpp"
#include "core/diagnostics/hook_context.hpp"
#include "core/diagnostics/seh_guard.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/parsers.hpp"
#include "core/hooks/registry/registry.hpp"
#include "core/hooks/registry/validate.hpp"

namespace hooks {
    namespace detail {
        template<typename HookList>
        struct RegistryOps {
            template<typename... Tags>
            static constexpr auto validate_all(hook_list<Tags...> /*unused*/) -> bool {
                return (sizeof(validate_hook_deps<Tags, HookList>) && ...);
            }

            static_assert(validate_all(HookList {}));

            template<typename Tag, typename... Tags>
            static constexpr auto hook_idx_in(hook_list<Tags...> /*unused*/) -> std::size_t {
                constexpr std::array matches = {std::is_same_v<Tag, Tags>...};
                for (std::size_t i = 0; i < sizeof...(Tags); ++i) {
                    if (matches.at(i)) {
                        return i;
                    }
                }
                return sizeof...(Tags);
            }

            template<typename Tag>
            static constexpr std::size_t hook_idx = hook_idx_in<Tag>(HookList {});

            template<typename Tag>
            struct DepIndices {
                static constexpr auto compute_hard() {
                    return []<typename... Deps>(dep_list<Deps...>) -> auto {
                        return std::array<std::size_t, sizeof...(Deps)> {hook_idx<Deps>...};
                    }(typename HookTraits<Tag>::hard_deps {});
                }
                static constexpr auto compute_soft() {
                    return []<typename... Deps>(dep_list<Deps...>) -> auto {
                        return std::array<std::size_t, sizeof...(Deps)> {hook_idx<Deps>...};
                    }(typename HookTraits<Tag>::soft_deps {});
                }
                static constexpr auto hard = compute_hard();
                static constexpr auto soft = compute_soft();
            };

            template<typename... Tags>
            static constexpr auto hook_list_size(hook_list<Tags...> /*unused*/) -> std::size_t {
                return sizeof...(Tags);
            }

            static constexpr std::size_t N = hook_list_size(HookList {});

            template<typename... Tags>
            static constexpr auto build_hard_deps(hook_list<Tags...> /*unused*/) {
                return std::array<std::span<const std::size_t>, sizeof...(Tags)> {
                    DepIndices<Tags>::hard...};
            }

            template<typename... Tags>
            static constexpr auto build_soft_deps(hook_list<Tags...> /*unused*/) {
                return std::array<std::span<const std::size_t>, sizeof...(Tags)> {
                    DepIndices<Tags>::soft...};
            }

            static constexpr auto topo_sort()
                -> std::pair<std::array<std::size_t, N>, std::size_t> {
                auto hard = build_hard_deps(HookList {});
                auto soft = build_soft_deps(HookList {});

                std::array<std::size_t, N>                in_degree {};
                std::array<std::array<std::size_t, N>, N> adj {};
                std::array<std::size_t, N>                adj_count {};

                for (std::size_t i = 0; i < N; ++i) {
                    for (auto dep : hard.at(i)) {
                        adj.at(dep).at(adj_count.at(dep)++) = i;
                        in_degree.at(i)++;
                    }
                    for (auto dep : soft.at(i)) {
                        adj.at(dep).at(adj_count.at(dep)++) = i;
                        in_degree.at(i)++;
                    }
                }

                std::array<std::size_t, N> queue {};
                std::size_t                front = 0;
                std::size_t                back  = 0;

                for (std::size_t i = 0; i < N; ++i) {
                    if (in_degree.at(i) == 0) {
                        queue.at(back++) = i;
                    }
                }

                std::array<std::size_t, N> order {};
                std::size_t                count = 0;

                while (front < back) {
                    auto u            = queue.at(front++);
                    order.at(count++) = u;
                    for (std::size_t i = 0; i < adj_count.at(u); ++i) {
                        auto v = adj.at(u).at(i);
                        if (--in_degree.at(v) == 0) {
                            queue.at(back++) = v;
                        }
                    }
                }

                return {order, count};
            }

            static constexpr auto sorted_result = topo_sort();
            static_assert(sorted_result.second == N, "Dependency cycle detected in hook graph");
            static constexpr auto install_order = sorted_result.first;

            struct HookOps {
                std::string_view             name;
                std::size_t                  index {};
                std::span<const std::size_t> hard_deps;
                std::span<const std::size_t> soft_deps;

                void (*load_config)(Registry<HookList> &r, mINI::INIStructure &ini) {};
                void (*load_enabled)(Registry<HookList> &r, mINI::INIStructure &ini) {};
                bool (*check_required_fn)(const void *addrs) {};
                bool (*do_install_fn)(const void *addrs) {};
                void (*set_installed)(Registry<HookList> &r, bool val) {};
                void (*set_enabled)(Registry<HookList> &r, bool val) {};
                bool (*is_enabled)(const Registry<HookList> &r) {};
                bool (*is_installed)(const Registry<HookList> &r) {};
                void (*call_on_reload)(Registry<HookList> &r) {};
            };

            template<typename Tag, typename Addrs = void>
            static auto make_ops() -> HookOps {
                return HookOps {
                    .name      = HookTraits<Tag>::name,
                    .index     = hook_idx<Tag>,
                    .hard_deps = DepIndices<Tag>::hard,
                    .soft_deps = DepIndices<Tag>::soft,

                    .load_config = [](Registry<HookList> &r, mINI::INIStructure &ini) -> void {
                        r.template config<Tag>().load_all(ini);
                    },
                    .load_enabled = [](Registry<HookList> &r, mINI::INIStructure &ini) -> void {
                        if (!ini.has("Hooks")) {
                            return;
                        }
                        auto       &sec = ini["Hooks"];
                        std::string key(HookTraits<Tag>::name);
                        if (sec.has(key)) {
                            r.template set_enabled<Tag>(default_parser<bool> {}(sec[key]));
                        }
                    },

                    .check_required_fn = []() -> bool (*)(const void *) {
                        if constexpr (!std::is_void_v<Addrs>) {
                            return +[](const void *raw) -> bool {
                                const auto &addrs = *static_cast<const Addrs *>(raw);
                                return std::ranges::all_of(HookTraits<Tag>::required_patterns,
                                                           [&](auto f) -> bool {
                                                               return (addrs.*f).has_value();
                                                           });
                            };
                        } else {
                            return static_cast<bool (*)(const void *)>(nullptr);
                        }
                    }(),

                    .do_install_fn = []() -> bool (*)(const void *) {
                        if constexpr (!std::is_void_v<Addrs>) {
                            return +[](const void *raw) -> bool {
                                const auto &addrs = *static_cast<const Addrs *>(raw);
                                return HookTraits<Tag>::install(addrs);
                            };
                        } else {
                            return static_cast<bool (*)(const void *)>(nullptr);
                        }
                    }(),

                    .set_installed = [](Registry<HookList> &r, bool val) -> void {
                        r.template set_installed<Tag>(val);
                    },
                    .set_enabled = [](Registry<HookList> &r, bool val) -> void {
                        r.template set_enabled<Tag>(val);
                    },
                    .is_enabled = [](const Registry<HookList> &r) -> bool {
                        return r.template enabled<Tag>();
                    },
                    .is_installed = [](const Registry<HookList> &r) -> bool {
                        return r.template installed<Tag>();
                    },
                    .call_on_reload = [](Registry<HookList> &r) -> void {
                        if constexpr (HasOnReload<Tag>) {
                            HookTraits<Tag>::on_reload(r.template config<Tag>());
                        }
                    },
                };
            }

            template<typename Addrs = void, typename... Tags>
            static auto make_all_ops(hook_list<Tags...> /*unused*/)
                -> std::array<HookOps, sizeof...(Tags)> {
                return {make_ops<Tags, Addrs>()...};
            }

            static void apply_enabled_flags(Registry<HookList>           &reg,
                                            const std::array<HookOps, N> &ops,
                                            mINI::INIStructure           &ini) {
                for (const auto &op : ops) {
                    op.load_enabled(reg, ini);
                }
                std::array<bool, N> enabled_flags {};
                for (std::size_t i = 0; i < N; ++i) {
                    enabled_flags.at(i) = ops.at(i).is_enabled(reg);
                }
                cascade_disable(ops, enabled_flags);
                for (std::size_t i = 0; i < N; ++i) {
                    ops.at(i).set_enabled(reg, enabled_flags.at(i));
                }
            }

            static void
                cascade_disable(const std::array<HookOps, N> &ops, std::array<bool, N> &enabled) {
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (std::size_t i = 0; i < N; ++i) {
                        if (!enabled.at(i)) {
                            continue;
                        }
                        for (auto dep : ops.at(i).hard_deps) {
                            if (!enabled.at(dep)) {
                                log::get()->info(
                                    "Hook '{}': disabled (hard dependency '{}' is disabled)",
                                    ops.at(i).name,
                                    ops.at(dep).name);
                                enabled.at(i) = false;
                                changed       = true;
                                break;
                            }
                        }
                    }
                }
            }
        };
    } // namespace detail

    template<typename HookList>
    template<typename Addrs>
    void Registry<HookList>::install_all(const Addrs &addrs, mINI::INIStructure &ini) {
        using Ops = detail::RegistryOps<HookList>;

        static const auto ops = Ops::template make_all_ops<Addrs>(HookList {});

        log::get()->trace("install_all: loading configs");
        for (const auto &op : ops) {
            op.load_config(*this, ini);
        }

        log::get()->trace("install_all: loading enabled flags");
        Ops::apply_enabled_flags(*this, ops, ini);

        log::get()->trace("install_all: installing in dependency order");

        int installed_count = 0;
        for (auto idx : Ops::install_order) {
            const auto &op = ops.at(idx);

            if (!op.is_enabled(*this)) {
                log::get()->info("Hook '{}': disabled", op.name);
                continue;
            }

            if (!op.check_required_fn(&addrs)) {
                log::get()->warn("Hook '{}': missing required patterns, skipping", op.name);
                continue;
            }

            diagnostics::set_current_hook_name(op.name);
            if (diagnostics::guarded_install(op.do_install_fn, &addrs, op.name)) {
                op.set_installed(*this, true);
                log::get()->info("Hook '{}': installed", op.name);
                diagnostics::crash_journal::write_hook_installed(op.name);
                ++installed_count;
            } else {
                log::get()->warn("Hook '{}': install failed", op.name);
            }
            diagnostics::set_current_hook_name({});
        }

        log::get()->info("Initialization complete: {}/{} hooks installed", installed_count, Ops::N);
    }

    template<typename HookList>
    void Registry<HookList>::reload(mINI::INIStructure &ini) {
        using Ops = detail::RegistryOps<HookList>;

        static const auto ops = Ops::make_all_ops(HookList {});

        log::get()->info("Config reload...");

        for (const auto &op : ops) {
            op.load_config(*this, ini);
        }
        log::get()->trace("reload: configs loaded");

        Ops::apply_enabled_flags(*this, ops, ini);

        for (const auto &op : ops) {
            if (op.is_installed(*this) && op.is_enabled(*this)) {
                log::get()->trace("reload: calling on_reload for '{}'", op.name);
                op.call_on_reload(*this);
            }
        }

        log::get()->info("Config reloaded");
    }
} // namespace hooks
