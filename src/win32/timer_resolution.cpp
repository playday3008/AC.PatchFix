#include "core/win32/timer_resolution.hpp"

#include <atomic>

#include <Windows.h>
#include <timeapi.h>

namespace win32 {
    namespace {
        std::atomic<bool> g_raised {false};
    } // namespace

    auto raise_timer_resolution() -> bool {
        bool expected = false;
        if (!g_raised.compare_exchange_strong(expected, true)) {
            return true;
        }
        if (timeBeginPeriod(1) == TIMERR_NOERROR) {
            return true;
        }
        g_raised.store(false);
        return false;
    }

    void restore_timer_resolution() {
        bool expected = true;
        if (g_raised.compare_exchange_strong(expected, false)) {
            timeEndPeriod(1);
        }
    }
} // namespace win32
