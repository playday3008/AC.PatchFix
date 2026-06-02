#include <stop_token>
#include <string>

#include <Windows.h>

#include "logger.hpp" // IWYU pragma: keep

#include "diagnostics/crash_logger.hpp"
#include "diagnostics/crash_report.hpp"
#include "games/game_init.hpp"
#include "games/syndicate/registry.hpp"
#include "win32/pe.hpp"

namespace {
    void init_impl(HMODULE hModule, std::stop_token stop) {
        using G = games::Syndicate;

        auto dll_dir  = win32::get_module_path(hModule).parent_path();
        auto exe_name = win32::get_module_path(nullptr).filename().string();

        if (exe_name != games::game_data<G>::exe_name) {
            return;
        }

        auto name     = games::game_data<G>::name;
        auto log_name = std::string("AC.") + std::string(name) + ".PatchFix";

        log::init((dll_dir / (log_name + ".log")).string());
        diagnostics::init_log();
        log::get()->info("{} initializing for {}", log_name, exe_name);

        auto ini_path = dll_dir / (log_name + ".ini");
        init_game<G>(games::syndicate::registry(), ini_path, stop);
    }
} // namespace

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

void game_init(HMODULE hModule, std::stop_token stop) {
    __try {
        init_impl(hModule, stop);
    } __except (diagnostics::install_fault_filter(GetExceptionInformation(), "game_init")) {
    }
}

#pragma clang diagnostic pop
