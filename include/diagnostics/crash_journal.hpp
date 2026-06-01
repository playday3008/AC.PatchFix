#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace diagnostics::crash_journal {
    struct JournalState {
        bool                     clean {};
        std::vector<std::string> hooks_active;
        std::string              timestamp;
    };

    void open(std::string_view path);
    void write_session_start();
    void write_hook_installed(std::string_view hook_name);
    void write_init_complete();
    void write_session_clean();
    auto read_previous() -> JournalState;
    void close();
} // namespace diagnostics::crash_journal
