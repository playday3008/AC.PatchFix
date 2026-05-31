#include <string>

#include <Windows.h>

#include "logger.hpp" // IWYU pragma: keep

#include "games/game_init.hpp"
#include "games/syndicate/registry.hpp"

void game_init(HMODULE hModule) {
    using G = games::Syndicate;

    auto dll_dir  = get_module_path(hModule).parent_path();
    auto exe_name = get_module_path(nullptr).filename().string();

    if (exe_name != games::game_data<G>::exe_name) {
        return;
    }

    auto name     = games::game_data<G>::name;
    auto log_name = std::string("AC.") + std::string(name) + ".PatchFix";

    log::init((dll_dir / (log_name + ".log")).string());
    log::get()->info("{} initializing for {}", log_name, exe_name);

    auto ini_path = dll_dir / (log_name + ".ini");
    init_game<G>(games::syndicate::registry(), ini_path);
}
