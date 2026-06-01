#include "diagnostics/patch_registry.hpp"

#include <algorithm>
#include <shared_mutex>
#include <vector>

namespace diagnostics::patch_registry {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        std::shared_mutex       g_mutex;
        std::vector<PatchEntry> g_patches;
#pragma clang diagnostic pop
    } // namespace

    void register_patch(std::uintptr_t                base,
                        std::size_t                   size,
                        std::span<const std::uint8_t> original,
                        std::string_view              hook_name,
                        PatchType                     type) {
        PatchEntry entry;
        entry.base      = base;
        entry.size      = size;
        entry.hook_name = hook_name;
        entry.type      = type;

        auto copy_size = std::min(original.size(), entry.original_bytes.size());
        std::copy_n(original.data(), copy_size, entry.original_bytes.data());
        entry.original_size = static_cast<std::uint8_t>(copy_size);

        const std::unique_lock<std::shared_mutex> lock(g_mutex);
        auto it = std::ranges::lower_bound(g_patches, base, {}, &PatchEntry::base);
        g_patches.insert(it, entry);
    }

    auto find_patch(std::uintptr_t addr) -> const PatchEntry * {
        const std::shared_lock<std::shared_mutex> lock(g_mutex);
        auto it = std::ranges::upper_bound(g_patches, addr, {}, &PatchEntry::base);
        if (it != g_patches.begin()) {
            --it;
            if (addr >= it->base && addr < it->base + it->size) {
                return &*it;
            }
        }
        return nullptr;
    }

    auto find_nearby(std::uintptr_t addr, std::size_t threshold) -> const PatchEntry * {
        const std::shared_lock<std::shared_mutex> lock(g_mutex);
        auto it = std::ranges::lower_bound(g_patches, addr, {}, &PatchEntry::base);

        const PatchEntry *best      = nullptr;
        std::size_t       best_dist = threshold + 1;

        if (it != g_patches.begin()) {
            auto prev     = std::prev(it);
            auto past_end = prev->base + prev->size;
            if (addr >= past_end) {
                auto dist = addr - past_end;
                if (dist < best_dist) {
                    best_dist = dist;
                    best      = &*prev;
                }
            }
        }

        if (it != g_patches.end()) {
            if (it->base > addr) {
                auto dist = it->base - addr;
                if (dist < best_dist) {
                    best = &*it;
                }
            }
        }

        return best;
    }

    auto all_patches() -> std::span<const PatchEntry> {
        const std::shared_lock<std::shared_mutex> lock(g_mutex);
        return g_patches;
    }
} // namespace diagnostics::patch_registry
