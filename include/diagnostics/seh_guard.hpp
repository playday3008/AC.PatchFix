#pragma once

#include <atomic>
#include <string_view>
#include <type_traits>

#include <Windows.h>

#include <safetyhook/context.hpp>

#include "diagnostics/crash_report.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

namespace diagnostics {
    template<typename Functor>
    void guarded_callback(safetyhook::Context &regs) {
        static_assert(std::is_trivially_destructible_v<Functor>,
                      "Functor must be trivially destructible for SEH guard");

        static std::atomic<bool> s_faulted {false};
        if (s_faulted.load(std::memory_order_relaxed)) {
            return;
        }
        __try {
            Functor {}(regs);
        } __except (callback_fault_filter(GetExceptionInformation())) {
            s_faulted.store(true, std::memory_order_relaxed);
        }
    }

    inline auto
        guarded_install(bool (*fn)(const void *), const void *addrs, std::string_view hook_name)
            -> bool {
        __try {
            return fn(addrs);
        } __except (install_fault_filter(GetExceptionInformation(), hook_name)) {
            return false;
        }
    }
} // namespace diagnostics

#pragma clang diagnostic pop
