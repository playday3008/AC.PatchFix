#include "config/file_watcher.hpp"

#include <array>
#include <filesystem>
#include <stop_token>
#include <string>
#include <utility>

#include <version>
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202311L
#    include <memory>
#    define START_LIFETIME_AS_AVAILABLE
#endif

#include <Windows.h>

#include "logger.hpp" // IWYU pragma: keep

#include "win32/string.hpp"
#include "win32/unique_handle.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"

FileWatcher::FileWatcher(const std::filesystem::path &file_path, Callback on_change)
    : m_dir_path(file_path.parent_path().empty() ? std::filesystem::path(".")
                                                 : file_path.parent_path()),
      m_file_name(file_path.filename()),
      m_on_change(std::move(on_change)),
      m_thread([this](const std::stop_token &token) -> void { watch_loop(token); }) {}

void FileWatcher::watch_loop(const std::stop_token &token) {
    log::get()->trace("FileWatcher: opening directory {}", m_dir_path.string());
    win32::UniqueHandle<> dir_handle(
        CreateFileA(m_dir_path.string().c_str(),
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                    nullptr));
    if (!dir_handle) {
        log::get()->warn("FileWatcher: failed to open directory");
        return;
    }

    win32::UniqueHandle<win32::NullInvalid> stop_event(CreateEventA(nullptr, TRUE, FALSE, nullptr));
    std::stop_callback on_stop(token, [&] -> void { SetEvent(stop_event.get()); });

    alignas(DWORD) std::array<char, 4096> buffer {};
    OVERLAPPED                            overlapped {};

    win32::UniqueHandle<win32::NullInvalid> event_handle(
        CreateEventA(nullptr, TRUE, FALSE, nullptr));
    overlapped.hEvent = event_handle.get();

    while (!token.stop_requested()) {
        ResetEvent(overlapped.hEvent);
        BOOL ok = ReadDirectoryChangesW(dir_handle.get(),
                                        buffer.data(),
                                        static_cast<DWORD>(buffer.size()),
                                        FALSE,
                                        FILE_NOTIFY_CHANGE_LAST_WRITE,
                                        nullptr,
                                        &overlapped,
                                        nullptr);
        if (ok == FALSE) {
            log::get()->warn("FileWatcher: ReadDirectoryChangesW failed");
            break;
        }

        std::array events = {overlapped.hEvent, stop_event.get()};
        DWORD      wait   = WaitForMultipleObjects(2, events.data(), FALSE, INFINITE);
        if (wait != WAIT_OBJECT_0) {
            break;
        }

        DWORD bytes_returned = 0;
        if (GetOverlappedResult(dir_handle.get(), &overlapped, &bytes_returned, FALSE) == FALSE) {
            continue;
        }
        if (bytes_returned == 0) {
            continue;
        }

#if defined(START_LIFETIME_AS_AVAILABLE)
        auto *info = std::start_lifetime_as<FILE_NOTIFY_INFORMATION>(buffer.data());
#else
        auto *info = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buffer.data());
#endif
        for (;;) {
            std::string name =
                win32::wchar_to_utf8(static_cast<LPCWCH>(info->FileName),
                                     static_cast<int>(info->FileNameLength / sizeof(WCHAR)));

            if (std::filesystem::path(name) == m_file_name) {
                log::get()->trace("FileWatcher: change detected for {}", name);
                constexpr DWORD k_debounce_ms = 100;
                Sleep(k_debounce_ms);
                m_on_change();
                break;
            }

            if (info->NextEntryOffset == 0) {
                break;
            }
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(reinterpret_cast<char *>(info) +
                                                               info->NextEntryOffset);
        }
    }
}

#pragma clang diagnostic pop

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
