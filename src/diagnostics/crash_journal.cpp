#include "diagnostics/crash_journal.hpp"

#include <cstddef>

#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <Windows.h>

namespace diagnostics::crash_journal {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        HANDLE      g_file = INVALID_HANDLE_VALUE;
        std::string g_path;
#pragma clang diagnostic pop

        void write_line(std::string_view line) {
            if (g_file == INVALID_HANDLE_VALUE) {
                return;
            }
            DWORD written = 0;
            WriteFile(g_file, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
            WriteFile(g_file, "\n", 1, &written, nullptr);
            FlushFileBuffers(g_file);
        }

        auto now_iso() -> std::string {
            auto tp = std::chrono::system_clock::now();
            return std::format("{:%FT%T}", std::chrono::floor<std::chrono::seconds>(tp));
        }

        auto make_clean_state() -> JournalState {
            JournalState s;
            s.clean = true;
            return s;
        }

        auto read_file_contents(std::string_view path) -> std::string {
            HANDLE h = CreateFileA(std::string(path).c_str(),
                                   GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
            if (h == INVALID_HANDLE_VALUE) {
                return {};
            }

            LARGE_INTEGER size {};
            GetFileSizeEx(h, &size);
            if (size.QuadPart == 0 || size.QuadPart > 4096) {
                CloseHandle(h);
                return {};
            }

            std::string buf(static_cast<std::size_t>(size.QuadPart), '\0');
            DWORD       bytes_read = 0;
            ReadFile(h, buf.data(), static_cast<DWORD>(buf.size()), &bytes_read, nullptr);
            CloseHandle(h);
            buf.resize(bytes_read);
            return buf;
        }

        auto parse_journal(const std::string &content) -> JournalState {
            JournalState state;
            state.clean = false;

            std::string_view remaining(content);
            while (!remaining.empty()) {
                auto nl   = remaining.find('\n');
                auto line = (nl != std::string_view::npos) ? remaining.substr(0, nl) : remaining;
                remaining = (nl != std::string_view::npos) ? remaining.substr(nl + 1)
                                                           : std::string_view {};

                if (line.starts_with("session_start ")) {
                    state.timestamp = std::string(line.substr(14));
                    state.hooks_active.clear();
                    state.clean = false;
                } else if (line.starts_with("hook_installed ")) {
                    state.hooks_active.emplace_back(line.substr(15));
                } else if (line.starts_with("session_clean")) {
                    state.clean = true;
                }
            }

            return state;
        }
    } // namespace

    void open(std::string_view path) {
        g_path = std::string(path);
        g_file = CreateFileA(g_path.c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             nullptr,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                             nullptr);
    }

    void write_session_start() {
        if (g_file == INVALID_HANDLE_VALUE) {
            return;
        }
        SetFilePointer(g_file, 0, nullptr, FILE_BEGIN);
        SetEndOfFile(g_file);
        write_line("session_start " + now_iso());
    }

    void write_hook_installed(std::string_view hook_name) {
        write_line(std::string("hook_installed ") + std::string(hook_name));
    }

    void write_init_complete() {
        write_line("init_complete " + now_iso());
    }

    void write_session_clean() {
        write_line("session_clean " + now_iso());
    }

    auto read_previous() -> JournalState {
        if (g_path.empty()) {
            return {};
        }
        auto content = read_file_contents(g_path);
        if (content.empty()) {
            return make_clean_state();
        }
        return parse_journal(content);
    }

    void close() {
        if (g_file != INVALID_HANDLE_VALUE) {
            CloseHandle(g_file);
            g_file = INVALID_HANDLE_VALUE;
        }
    }
} // namespace diagnostics::crash_journal
