#include <memory>
#include <optional>
#include <thread>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/address_registry.hpp"
#include "core/diagnostics/crash_handler.hpp"
#include "core/diagnostics/crash_journal.hpp"
#include "core/vmp/integrity_bypass.hpp"

#include "games/game_init.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
auto watcher() -> std::unique_ptr<FileWatcher> & {
    static std::unique_ptr<FileWatcher> instance;
    return instance;
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
static std::optional<std::jthread> g_init_thread;
#pragma clang diagnostic pop

// NOLINTNEXTLINE(misc-use-internal-linkage, modernize-use-trailing-return-type)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*unused*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        diagnostics::register_plugin_module(hModule);
        diagnostics::install_veh();
        g_init_thread.emplace([hModule](const std::stop_token &stop) -> void {
            (void)vmp::install(GetModuleHandleW(nullptr));
            game_init(hModule, stop);
        });
    } else if (reason == DLL_PROCESS_DETACH) {
        watcher().reset();
        g_init_thread.reset();
        vmp::uninstall();
        diagnostics::crash_journal::write_session_clean();
        diagnostics::crash_journal::close();
        diagnostics::uninstall_veh();
        log::shutdown();
    }
    return TRUE;
}
