#pragma once

#include <exception>
#include <filesystem>
#include <memory>

#include <Windows.h>

#include <mini/ini.h>

#include "logger.hpp" // IWYU pragma: keep

#include "config/file_watcher.hpp"
#include "games/game_data.hpp"
#include "mem/protect.hpp"
#include "patterns/signatures.hpp"
#include "vmp/integrity_bypass.hpp"

auto watcher() -> std::unique_ptr<FileWatcher> &;

void game_init(HMODULE hModule);

template<typename G, typename Registry>
void init_game(Registry &registry, const std::filesystem::path &ini_path) {
    using Data  = games::game_data<G>;
    using Addrs = Data::ResolvedAddresses;

    if constexpr (games::game_is_vmprotect<G>) {
        mem::set_protect_method(mem::ProtectMethod::nt_protect);
        vmp::wait_for_unpack();
        vmp::wait_for_integrity_blocked();
        log::get()->info("VMP bypass active, .text writable via NtProtectVirtualMemory");
    }

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
