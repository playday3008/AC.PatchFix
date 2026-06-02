#include <stop_token>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/crash_logger.hpp"
#include "core/diagnostics/crash_report.hpp"

#include "games/game_init.hpp"
#include "games/rogue/registry.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

void game_init(HMODULE hModule, std::stop_token stop) {
    __try {
        game_init_impl<games::Rogue>(hModule, stop, games::rogue::registry());
    } __except (diagnostics::install_fault_filter(GetExceptionInformation(), "game_init")) {
        log::get()->critical("Fatal exception during init — plugin disabled");
    }
}

#pragma clang diagnostic pop
