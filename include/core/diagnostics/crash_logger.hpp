#pragma once

namespace spdlog {
    class logger;
} // namespace spdlog

namespace diagnostics {
    void init_log();
    auto log() -> spdlog::logger &;
} // namespace diagnostics
