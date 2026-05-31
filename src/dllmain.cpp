#include <memory>
#include <optional>
#include <thread>

#include <Windows.h>

#include "logger.hpp" // IWYU pragma: keep

#include "games/game_init.hpp"
#include "vmp/integrity_bypass.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
std::unique_ptr<FileWatcher>       g_watcher;
static std::optional<std::jthread> g_init_thread;
#pragma clang diagnostic pop

// NOLINTNEXTLINE(misc-use-internal-linkage, modernize-use-trailing-return-type)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*unused*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        vmp::install(GetModuleHandleW(nullptr));
        g_init_thread.emplace([hModule] -> void { game_init(hModule); });
    } else if (reason == DLL_PROCESS_DETACH) {
        g_watcher.reset();
        g_init_thread.reset();
        vmp::uninstall();
        log::shutdown();
    }
    return TRUE;
}
