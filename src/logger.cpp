#include "core/logger.hpp"

#include <cstddef>

#include <algorithm>
#include <flat_map>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Windows.h>

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace {
    auto null_logger() -> std::shared_ptr<spdlog::logger> {
        static const auto &inst =
            *new std::shared_ptr<spdlog::logger>([]() -> std::shared_ptr<spdlog::logger> {
                auto l = std::make_shared<spdlog::logger>("null");
                l->set_level(spdlog::level::off);
                return l;
            }());
        return inst;
    }

    auto to_display_name(std::string_view raw) -> std::string {
        std::string name(raw);
        std::ranges::replace(name, '/', '.');
        std::ranges::replace(name, '\\', '.');
        return name;
    }

    void make_logger(std::string_view                 raw_name,
                     std::vector<spdlog::sink_ptr>   &sinks,
                     std::shared_ptr<spdlog::logger> &out) {
        const std::string name = to_display_name(raw_name);
        auto logger            = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::info);
        spdlog::register_logger(logger);
        out = std::move(logger);
    }
} // namespace

struct log::Impl {
    std::vector<spdlog::sink_ptr>                                            sinks;
    std::mutex                                                               mutex;
    std::flat_map<std::string, std::shared_ptr<spdlog::logger>, std::less<>> loggers;
};

namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
    std::unique_ptr<class log> g_instance;
#pragma clang diagnostic pop
} // namespace

log::log(std::string_view path, std::size_t maxSize, std::size_t maxFiles)
    : m(std::make_unique<Impl>()) {
    const std::string p(path);
    try {
        m->sinks.push_back(
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(p, maxSize, maxFiles, true));
    } catch (const spdlog::spdlog_ex &) {
        // rotate_on_open failed — previous process was killed and Wine
        // hasn't released the file handle yet (no FILE_SHARE_DELETE).
        // Truncate via Win32 (succeeds because the old handle allows
        // FILE_SHARE_WRITE), then open without rotation.
        HANDLE h = CreateFileA(p.c_str(),
                               GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr,
                               TRUNCATE_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
        }
        m->sinks.push_back(
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(p, maxSize, maxFiles));
    }
    m->sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
}

log::~log() {
    try {
        for (const auto [_, logger] : m->loggers) {
            if (logger) {
                logger->flush();
            }
        }
        spdlog::shutdown();
    } catch (...) { // NOLINT(bugprone-empty-catch)
    }

    m->loggers.clear();
    m->sinks.clear();
}

void log::init(std::string_view path, std::size_t maxSize, std::size_t maxFiles) {
    if (g_instance) {
        return;
    }
    g_instance = std::unique_ptr<log>(new class log(path, maxSize, maxFiles));
}

void log::shutdown() {
    g_instance.reset();
}

auto log::get(std::string_view name) -> std::shared_ptr<spdlog::logger> {
    if (g_instance == nullptr) {
        return null_logger();
    }

    const std::scoped_lock<std::mutex> lock(g_instance->m->mutex);

    if (auto it = g_instance->m->loggers.find(name); it != g_instance->m->loggers.end()) {
        return it->second;
    }

    auto &slot = g_instance->m->loggers[std::string(name)];
    make_logger(name, g_instance->m->sinks, slot);
    return slot;
}
