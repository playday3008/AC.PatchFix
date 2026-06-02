#pragma once

#include <exception>
#include <filesystem>
#include <memory>
#include <stop_token>
#include <string>

#include <Windows.h>

#include <mini/ini.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/config/file_watcher.hpp"
#include "core/diagnostics/crash_journal.hpp"
#include "core/diagnostics/crash_logger.hpp"
#include "core/mem/protect.hpp"
#include "core/patterns/signatures.hpp"
#include "core/version.hpp"
#include "core/vmp/integrity_bypass.hpp"
#include "core/win32/pe.hpp"

#include "games/game_data.hpp"

auto watcher() -> std::unique_ptr<FileWatcher> &;

void game_init(HMODULE hModule, const std::stop_token &stop);

template<typename G, typename Registry>
void init_game(Registry                    &registry,
               const std::filesystem::path &ini_path,
               const std::stop_token       &stop) {
    using Data  = games::game_data<G>;
    using Addrs = Data::ResolvedAddresses;

    if constexpr (games::game_is_vmprotect<G>) {
        mem::set_protect_method(mem::ProtectMethod::nt_protect);
        vmp::wait_for_unpack(stop);
        vmp::wait_for_integrity_blocked(stop);
        log::get()->info("VMP bypass active, .text writable via NtProtectVirtualMemory");
    }

    auto journal_path = ini_path.parent_path() /
                        (std::string("AC.") + std::string(Data::name) + ".PatchFix.journal");
    diagnostics::crash_journal::open(journal_path.string());

    auto prev = diagnostics::crash_journal::read_previous();
    if (!prev.clean && !prev.hooks_active.empty()) {
        log::get()->warn("Previous session did not shut down cleanly (started {})", prev.timestamp);
        std::string hook_list;
        for (const auto &h : prev.hooks_active) {
            if (!hook_list.empty()) {
                hook_list += ", ";
            }
            hook_list += h;
        }
        log::get()->warn("Hooks active at crash: {}", hook_list);
        log::get()->warn("Check the log file from the previous session for crash details");
    }

    diagnostics::crash_journal::write_session_start();

    const mINI::INIFile file(ini_path.string());
    mINI::INIStructure  ini;
    if (!file.read(ini)) {
        log::get()->warn("Failed to read INI, using defaults");
    }
    log::get()->trace("INI loaded from {}", ini_path.string());

    Addrs addrs;
    bool  all_found = true;
    for (const auto &entry : Data::scan_entries) {
        auto result = patterns::find_unique(entry.name, entry.bytes, entry.offset);
        if (result) {
            addrs.*(entry.field) = *result;
            log::get()->trace("Pattern {}: 0x{:X}", entry.name, *result);
        } else {
            all_found = false;
            log::get()->warn("{}", result.error());
        }
    }
    log::get()->info("Pattern scan: {}", all_found ? "all found" : "some missing");

    try {
        registry.install_all(addrs, ini);
    } catch (const std::exception &e) {
        log::get()->critical("install_all failed: {}", e.what());
        return;
    } catch (...) {
        log::get()->critical("install_all failed: unknown exception");
        return;
    }
    log::get()->info("Hook registry initialized");

    diagnostics::crash_journal::write_init_complete();

    if constexpr (games::game_is_vmprotect<G>) {
        vmp::uninstall();
    }

    watcher() = std::make_unique<FileWatcher>(ini_path, [ini_path, &registry] -> auto {
        log::get()->info("INI change detected, reloading...");
        const mINI::INIFile f(ini_path.string());
        mINI::INIStructure  data;
        if (f.read(data)) {
            registry.reload(data);
        } else {
            log::get()->warn("Config reload failed: could not read INI");
        }
    });
    log::get()->info("File watcher started for {}", ini_path.string());
}

template<typename G, typename Registry>
void game_init_impl(HMODULE hModule, const std::stop_token &stop, Registry &registry) {
    auto dll_dir  = win32::get_module_path(hModule).parent_path();
    auto exe_name = win32::get_module_path(nullptr).filename().string();

    if (exe_name != games::game_data<G>::exe_name) {
        return;
    }

    auto name     = games::game_data<G>::name;
    auto log_name = std::string("AC.") + std::string(name) + ".PatchFix";

    log::init((dll_dir / (log_name + ".log")).string());
    diagnostics::init_log();
    log::get()->info("{} v{} initializing for {}", log_name, version::string, exe_name);

    auto ini_path = dll_dir / (log_name + ".ini");
    init_game<G>(registry, ini_path, stop);
}
