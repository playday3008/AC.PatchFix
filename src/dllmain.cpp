#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <variant>

#include <Windows.h>

#include <mini/ini.h>

#include "logger.hpp" // IWYU pragma: keep

#include "config/file_watcher.hpp"
#include "games/variant.hpp"
#include "patterns/signatures.hpp"
#include "win32/string.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
static std::unique_ptr<FileWatcher> g_watcher;
static std::optional<std::jthread>  g_init_thread;
#pragma clang diagnostic pop

static auto get_module_path(HMODULE hModule) -> std::filesystem::path {
    std::array<WCHAR, MAX_PATH> buf {};
    DWORD                       len = GetModuleFileNameW(hModule, buf.data(), MAX_PATH);
    return std::filesystem::path(win32::wchar_to_utf8(buf.data(), static_cast<int>(len)));
}

template<typename G>
static auto try_detect(std::string_view exe) -> std::optional<games::RegistryVariant> {
    if (exe == games::game_data<G>::exe_name) {
        return &games::rogue::g_registry;
    }
    return std::nullopt;
}

static auto detect_game(std::string_view exe) -> std::optional<games::RegistryVariant> {
    if (auto v = try_detect<games::Rogue>(exe)) {
        return v;
    }
    return std::nullopt;
}

template<typename G, typename Registry>
static void init_game(Registry &registry, const std::filesystem::path &ini_path) {
    using Data  = games::game_data<G>;
    using Addrs = typename Data::ResolvedAddresses;

    mINI::INIFile      file(ini_path.string());
    mINI::INIStructure ini;
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

    registry.install_all(addrs, ini);
    log::get()->trace("Hook registry initialized");

    g_watcher = std::make_unique<FileWatcher>(ini_path, [ini_path, &registry] -> auto {
        log::get()->info("INI change detected, reloading...");
        mINI::INIFile      f(ini_path.string());
        mINI::INIStructure data;
        if (f.read(data)) {
            registry.reload(data);
        } else {
            log::get()->warn("Config reload failed: could not read INI");
        }
    });
    log::get()->info("File watcher started for {}", ini_path.string());
}

static void init(HMODULE hModule) {
    auto dll_dir  = get_module_path(hModule).parent_path();
    auto exe_name = get_module_path(nullptr).filename().string();

    auto variant = detect_game(exe_name);
    if (!variant) {
        return;
    }

    std::visit(
        [&](auto *registry) -> auto {
            using Registry = std::remove_pointer_t<decltype(registry)>;
            using HookList = typename Registry::hook_list_type;

            constexpr auto deduce_game = []<typename G, typename... Rest>(
                                             hooks::hook_list<::GameStateHook<G>, Rest...>) -> G {
                return {};
            };
            using G = decltype(deduce_game(HookList {}));

            auto name     = games::game_data<G>::name;
            auto log_name = std::string("AC.") + std::string(name) + ".PatchFix";

            log::init((dll_dir / (log_name + ".log")).string());
            log::get()->info("{} initializing for {}", log_name, exe_name);

            auto ini_path = dll_dir / (log_name + ".ini");
            init_game<G>(*registry, ini_path);
        },
        *variant);
}

// NOLINTNEXTLINE(misc-use-internal-linkage, modernize-use-trailing-return-type)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*unused*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        g_init_thread.emplace([hModule] -> void { init(hModule); });
    } else if (reason == DLL_PROCESS_DETACH) {
        g_watcher.reset();
        g_init_thread.reset();
        log::shutdown();
    }
    return TRUE;
}
