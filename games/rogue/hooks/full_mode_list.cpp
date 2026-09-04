#include "games/rogue/hooks/full_mode_list.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include <dxgi.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::FullModeListHook;

        // Renderer fields the original list builder works on.
        constexpr std::uintptr_t k_output      = 0x18;
        constexpr std::uintptr_t k_format      = 0x150;
        constexpr std::uintptr_t k_mode_vector = 0x1A8;
        constexpr std::uintptr_t k_mode_cap    = 0x1B0;
        constexpr std::uintptr_t k_mode_count  = 0x1B2;

        constexpr std::uint32_t k_capacity_mask = 0x3FFF;

        // The engine drops anything smaller than this before the list is built.
        constexpr std::uint32_t k_min_width  = 800;
        constexpr std::uint32_t k_min_height = 600;

        // char reserve(vector *self, uint32_t capacity, void *allocator)
        using ReserveFn = char (*)(void *, std::uint32_t, void *);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        ReserveFn g_reserve        = nullptr;
        void    **g_allocator_slot = nullptr;

        auto refresh_hz(const DXGI_MODE_DESC &mode) -> std::uint32_t {
            if (mode.RefreshRate.Denominator == 0) {
                return 0;
            }
            const auto num = static_cast<float>(mode.RefreshRate.Numerator);
            const auto den = static_cast<float>(mode.RefreshRate.Denominator);
            return static_cast<std::uint32_t>(std::lround(num / den));
        }

        // The engine keeps one entry per resolution, preferring whichever mode sits
        // within 1 Hz of 60, so a 165 Hz panel loses every native-refresh entry and
        // the saved mode no longer resolves to an index. Fill the list with every
        // mode the output reports instead, in the order the lookup expects.
        void rebuild(std::uintptr_t renderer) {
            *reinterpret_cast<std::uint16_t *>(renderer + k_mode_count) = 0;

            auto *output = *reinterpret_cast<IDXGIOutput **>(renderer + k_output);
            if (output == nullptr) {
                return;
            }

            const auto format = *reinterpret_cast<DXGI_FORMAT *>(renderer + k_format);

            UINT available = 0;
            if (FAILED(output->GetDisplayModeList(format,
                                                  DXGI_ENUM_MODES_SCALING,
                                                  &available,
                                                  nullptr)) ||
                available == 0) {
                log::get()->warn("FullModeList: output reported no modes");
                return;
            }

            std::vector<DXGI_MODE_DESC> modes(available);
            if (FAILED(output->GetDisplayModeList(format,
                                                  DXGI_ENUM_MODES_SCALING,
                                                  &available,
                                                  modes.data()))) {
                log::get()->error("FullModeList: GetDisplayModeList failed");
                return;
            }
            modes.resize(available);

            std::erase_if(modes, [](const DXGI_MODE_DESC &mode) -> bool {
                return mode.Width < k_min_width || mode.Height < k_min_height;
            });
            if (modes.empty()) {
                log::get()->warn("FullModeList: every reported mode was below {}x{}",
                                 k_min_width,
                                 k_min_height);
                return;
            }

            // The index lookup is a binary search over (width, height, refresh).
            std::ranges::sort(modes, [](const DXGI_MODE_DESC &a, const DXGI_MODE_DESC &b) -> bool {
                if (a.Width != b.Width) {
                    return a.Width < b.Width;
                }
                if (a.Height != b.Height) {
                    return a.Height < b.Height;
                }
                return refresh_hz(a) < refresh_hz(b);
            });

            const auto capacity = *reinterpret_cast<std::uint32_t *>(renderer + k_mode_cap) &
                                  k_capacity_mask;
            if (modes.size() > capacity) {
                g_reserve(reinterpret_cast<void *>(renderer + k_mode_vector),
                          static_cast<std::uint32_t>(modes.size()),
                          *g_allocator_slot);
            }

            auto *dst = *reinterpret_cast<DXGI_MODE_DESC **>(renderer + k_mode_vector);
            if (dst == nullptr) {
                log::get()->error("FullModeList: mode list allocation failed");
                return;
            }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
            std::memcpy(dst, modes.data(), modes.size() * sizeof(DXGI_MODE_DESC));
#pragma clang diagnostic pop
            *reinterpret_cast<std::uint16_t *>(renderer + k_mode_count) =
                static_cast<std::uint16_t>(modes.size());

            log::get()->info("FullModeList: published {} modes ({}x{} @{}Hz first, {}x{} @{}Hz "
                             "last)",
                             modes.size(),
                             modes.front().Width,
                             modes.front().Height,
                             refresh_hz(modes.front()),
                             modes.back().Width,
                             modes.back().Height,
                             refresh_hz(modes.back()));
        }

        struct BuildModeList {
            [[maybe_unused]] static constexpr std::string_view name = "FullModeList";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                rebuild(regs.rcx);

                // The mid hook stub resumes with `mov rsp, trampoline_rsp; ret`, so
                // pointing it at the entry rsp returns to the caller and pops the
                // return address exactly as the original epilogue would.
                regs.rax            = 1;
                regs.trampoline_rsp = regs.rsp;
            }
        };

        auto rel32_target(std::uintptr_t operand, std::uintptr_t next_insn) -> std::uintptr_t {
            const auto rel = *reinterpret_cast<const std::int32_t *>(operand);
            return static_cast<std::uintptr_t>(static_cast<std::intptr_t>(next_insn) + rel);
        }
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        // mov r8, cs:allocator | mov edx, ecx | lea rcx, [r14+1A8h] | call reserve
        const auto site = addrs.mode_list_reserve_site.value();

        // The allocator global is filled in during engine startup, so keep the slot
        // and read it when the list is actually rebuilt.
        g_allocator_slot = reinterpret_cast<void **>(rel32_target(site + 3, site + 7));
        g_reserve        = reinterpret_cast<ReserveFn>(rel32_target(site + 0x11, site + 0x15));
        auto address     = addrs.mode_list_build.value();

        auto hook_result = mem::make_hook<BuildModeList>(address);
        if (!hook_result) {
            log::get()->error("FullModeList: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("FullModeList: installed at 0x{:X} (reserve=0x{:X}, allocator slot "
                         "0x{:X})",
                         address,
                         reinterpret_cast<std::uintptr_t>(g_reserve),
                         reinterpret_cast<std::uintptr_t>(g_allocator_slot));
        return true;
    }
} // namespace hooks
