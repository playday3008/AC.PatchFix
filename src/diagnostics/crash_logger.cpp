#include "diagnostics/crash_logger.hpp"

#include <atomic>
#include <memory>

#include <spdlog/spdlog.h>

#include "logger.hpp" // IWYU pragma: keep

namespace diagnostics {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        std::shared_ptr<spdlog::logger> g_crash_logger_owner;
#pragma clang diagnostic pop
        std::atomic<spdlog::logger *> g_crash_logger {nullptr};
    } // namespace

    void init_log() {
        g_crash_logger_owner = log::get();
        g_crash_logger.store(g_crash_logger_owner.get(), std::memory_order_release);
    }

    auto log() -> spdlog::logger & {
        auto *logger = g_crash_logger.load(std::memory_order_acquire);
        if (logger != nullptr) {
            return *logger;
        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
        static auto s_null = []() -> spdlog::logger {
            spdlog::logger l("null");
            l.set_level(spdlog::level::off);
            return l;
        }();
#pragma clang diagnostic pop
        return s_null;
    }
} // namespace diagnostics
