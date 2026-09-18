#include <memory>
#include <thread>
#include <tuple>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/address_registry.hpp"
#include "core/diagnostics/crash_handler.hpp"
#include "core/diagnostics/crash_journal.hpp"
#include "core/vmp/integrity_bypass.hpp"
#include "core/win32/timer_resolution.hpp"

#include "games/game_init.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
auto watcher() -> std::unique_ptr<FileWatcher> & {
    static std::unique_ptr<FileWatcher> instance;
    return instance;
}
#pragma clang diagnostic pop

namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
    // The init thread must never be joined from DllMain (see the detach path
    // below), and std::jthread joins in its destructor, so ownership is released
    // rather than destroyed at a point where joining would deadlock.
    std::unique_ptr<std::jthread> g_init_thread;
#pragma clang diagnostic pop
} // namespace

// NOLINTNEXTLINE(misc-use-internal-linkage, modernize-use-trailing-return-type)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        diagnostics::register_plugin_module(hModule);
        diagnostics::install_veh();
        g_init_thread = std::make_unique<std::jthread>(
            [hModule](const std::stop_token &stop) -> void { game_init(hModule, stop); });
    } else if (reason == DLL_PROCESS_DETACH) {
        // Neither thread may be joined here. DllMain runs under the loader lock,
        // and both the init thread (LoadLibraryA for dbghelp) and the watcher
        // thread (spdlog, registry reload) can block on it, so joining deadlocks.
        // On process exit (lpReserved non-null) every other thread has already
        // been terminated, so a join could never complete at all.
        if (g_init_thread) {
            g_init_thread->request_stop();
            std::ignore = g_init_thread.release();
        }
        if (watcher()) {
            watcher()->stop();
            std::ignore = watcher().release();
        }

        // On process exit the OS reclaims everything, and the remaining calls all
        // touch state the terminated threads may have been holding locks over.
        if (lpReserved != nullptr) {
            return TRUE;
        }

        vmp::uninstall();
        win32::restore_timer_resolution();
        diagnostics::crash_journal::write_session_clean();
        diagnostics::crash_journal::close();
        diagnostics::uninstall_veh();
        log::shutdown();
    }
    return TRUE;
}
