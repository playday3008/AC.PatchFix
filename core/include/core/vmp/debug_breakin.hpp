#pragma once

namespace vmp {
    // VMProtect's debugger check redirects ntdll!DbgUiRemoteBreakin to a stub
    // whose whole body is TerminateProcess(GetCurrentProcess(), 0xDEADC0DE), so
    // the thread the OS injects on DebugActiveProcess kills the process instead
    // of raising the breakpoint a debugger expects. That thread is created by
    // the debug subsystem rather than by the process.
    //
    // Restoring the prologue from the copy of ntdll on disk puts the function
    // back to what the loader mapped, so an attach behaves normally.
    enum class BreakinState : unsigned char {
        clean,       //!< no patch present
        restored,    //!< was patched, original bytes written back
        patched,     //!< patched, and the write failed
        unavailable, //!< could not read ntdll or locate the export
    };

    auto restore_debug_breakin() -> BreakinState;

    [[nodiscard]] auto breakin_state_name(BreakinState state) -> const char *;
} // namespace vmp
