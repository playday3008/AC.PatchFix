#include "core/vmp/integrity_bypass.hpp"

#include <cstdint>

#include <atomic>
#include <chrono>
#include <stop_token>
#include <thread>
#include <utility>

#include <Windows.h>
#include <winternl.h>

#include <safetyhook/inline_hook.hpp>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/vmp/detail/pe_sections.hpp"
#include "core/vmp/detail/thread_intercept.hpp"

namespace vmp {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
    namespace {
        std::atomic<bool> g_active {false};
        // Written once in install() before the inline hook is armed.
        // hk_create_thread can only run after the hook is live, and safetyhook's
        // InlineHook::create provides a full memory barrier, so this is safe.
        detail::VmpSections    g_sections;
        safetyhook::InlineHook g_create_thread_hook;
        std::atomic<int>       g_blocked_count {0};

        auto WINAPI empty_thread(LPVOID /*param*/) -> DWORD {
            return 0;
        }

        auto WINAPI hk_create_thread(LPSECURITY_ATTRIBUTES  attrs,
                                     SIZE_T                 stack_size,
                                     LPTHREAD_START_ROUTINE start,
                                     LPVOID                 param,
                                     DWORD                  flags,
                                     LPDWORD                thread_id) -> HANDLE {
            if (detail::is_vmp_thread(start, g_sections)) {
                auto addr = reinterpret_cast<std::uintptr_t>(start);
                log::get()->info("[VMP] Blocked integrity thread at 0x{:X}", addr);
                start = &empty_thread;
                g_blocked_count.fetch_add(1, std::memory_order_relaxed);
            }
            return g_create_thread_hook
                .call<HANDLE>(attrs, stack_size, start, param, flags, thread_id);
        }
    } // namespace
#pragma clang diagnostic pop

    auto active() -> bool {
        return g_active.load(std::memory_order_acquire);
    }

    auto install(HMODULE game_module) -> bool {
        g_sections = detail::find_vmp_sections(game_module);

        if (!g_sections.has_vmp()) {
            return false;
        }

        NtCurrentTeb()->ProcessEnvironmentBlock->BeingDebugged = 0;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
        auto *ct = reinterpret_cast<decltype(CreateThread) *>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateThread"));
#pragma clang diagnostic pop

        if (ct == nullptr) {
            return false;
        }

        auto result =
            safetyhook::InlineHook::create(ct, reinterpret_cast<void *>(&hk_create_thread));

        if (!result) {
            return false;
        }

        g_create_thread_hook = std::move(*result);
        g_active.store(true, std::memory_order_release);
        return true;
    }

    void wait_for_unpack(std::stop_token stop) {
        if (!g_active.load(std::memory_order_acquire)) {
            return;
        }

        const auto *text_byte =
            reinterpret_cast<volatile const std::uint8_t *>(g_sections.text->base);
        if (*text_byte != 0) {
            log::get()->trace("[VMP] .text already unpacked");
            return;
        }

        log::get()->trace("[VMP] Polling .text for unpack...");
        constexpr int timeout_seconds = 30;
        for (int i = 0; i < timeout_seconds * 100; ++i) {
            if (stop.stop_requested()) {
                log::get()->warn("[VMP] Unpack wait cancelled");
                return;
            }
            if (*text_byte != 0) {
                log::get()->info("[VMP] .text unpacked after ~{}ms", i * 10);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        log::get()->warn("[VMP] Unpack timeout after {}s", timeout_seconds);
    }

    void wait_for_integrity_blocked(std::stop_token stop) {
        if (!g_active.load(std::memory_order_acquire)) {
            return;
        }

        constexpr int timeout_seconds = 30;
        for (int i = 0; i < timeout_seconds * 10; ++i) {
            if (stop.stop_requested()) {
                log::get()->warn("[VMP] Integrity wait cancelled");
                return;
            }
            if (g_blocked_count.load(std::memory_order_relaxed) > 0) {
                log::get()->info("[VMP] Integrity thread blocked after ~{}ms", i * 100);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        log::get()->warn("[VMP] No integrity thread caught after {}s", timeout_seconds);
    }

    void uninstall() {
        if (!g_active.load(std::memory_order_acquire)) {
            return;
        }

        g_create_thread_hook.reset();
        g_active.store(false, std::memory_order_release);
        log::get()->info("[VMP] Hooks removed");
    }
} // namespace vmp
