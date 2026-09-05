#pragma once

namespace win32 {
    // Raises the process timer resolution to 1 ms so Sleep() stops rounding up to
    // the ~15.6 ms default quantum. Idempotent: repeated calls raise it once.
    auto raise_timer_resolution() -> bool;

    // Drops the resolution raised by raise_timer_resolution(). No-op if unraised.
    void restore_timer_resolution();
} // namespace win32
