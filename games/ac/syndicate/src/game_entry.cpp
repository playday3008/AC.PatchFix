#include <stop_token>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/crash_logger.hpp"
#include "core/diagnostics/crash_report.hpp"

#include "games/ac/syndicate/registry.hpp"
#include "games/game_init.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

void game_init(HMODULE hModule, const std::stop_token &stop) {
    __try {
        game_init_impl<games::ac::Syndicate>(hModule, stop, games::ac::syndicate::registry());
    } __except (diagnostics::install_fault_filter(GetExceptionInformation(), "game_init")) {
        log::get()->critical("Fatal exception during init — plugin disabled");
    }
}

#pragma clang diagnostic pop
