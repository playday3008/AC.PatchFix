#include "core/config/file_watcher.hpp"

#include <cstddef>

#include <array>
#include <atomic>
#include <filesystem>
#include <span>
#include <stop_token>
#include <string>
#include <utility>
#include <version>
#if defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202311L
#    include <memory>
#    define START_LIFETIME_AS_AVAILABLE
#endif

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/win32/string.hpp"
#include "core/win32/unique_handle.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"

FileWatcher::FileWatcher(const std::filesystem::path &file_path, Callback on_change)
    : m_dir_path(file_path.parent_path().empty() ? std::filesystem::path(".")
                                                 : file_path.parent_path()),
      m_file_name(file_path.filename()),
      m_on_change(std::move(on_change)),
      m_thread([this](const std::stop_token &token) -> void { watch_loop(token); }) {}

void FileWatcher::stop() {
    m_stopping.store(true, std::memory_order_release);
    m_thread.request_stop();
}

void FileWatcher::watch_loop(const std::stop_token &token) {
    log::get()->trace("FileWatcher: opening directory {}", m_dir_path.string());
    const win32::UniqueHandle<> dir_handle(
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
    const std::stop_callback on_stop(token, [&] -> void { SetEvent(stop_event.get()); });

    alignas(DWORD) std::array<std::byte, 4096> buffer {};
    OVERLAPPED                                 overlapped {};

    const win32::UniqueHandle<win32::NullInvalid> event_handle(
        CreateEventA(nullptr, TRUE, FALSE, nullptr));
    overlapped.hEvent = event_handle.get();

    while (!token.stop_requested()) {
        ResetEvent(overlapped.hEvent);
        const BOOL ok = ReadDirectoryChangesW(dir_handle.get(),
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

        std::array  events = {overlapped.hEvent, stop_event.get()};
        const DWORD wait   = WaitForMultipleObjects(2, events.data(), FALSE, INFINITE);
        if (wait == WAIT_FAILED) {
            log::get()->warn("FileWatcher: wait failed, hot-reload stopped");
            break;
        }
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

        if (records_name_our_file(std::span(buffer).first(bytes_returned))) {
            // Coalesce the burst of writes an editor makes when saving. Waiting on
            // the stop event rather than sleeping keeps shutdown prompt and stops
            // us from running the callback on the way out.
            constexpr DWORD k_debounce_ms = 100;
            if (WaitForSingleObject(stop_event.get(), k_debounce_ms) == WAIT_OBJECT_0) {
                return;
            }
            if (m_stopping.load(std::memory_order_acquire)) {
                return;
            }
            m_on_change();
        }
    }
}

auto FileWatcher::records_name_our_file(std::span<const std::byte> records) const -> bool {
    // Offsets come from the kernel, but the buffer is a fixed-size local, so every
    // record is bounds-checked against what ReadDirectoryChangesW actually wrote.
    std::size_t offset = 0;
    while (offset + sizeof(FILE_NOTIFY_INFORMATION) <= records.size()) {
#ifdef START_LIFETIME_AS_AVAILABLE
        const auto *info =
            std::start_lifetime_as<const FILE_NOTIFY_INFORMATION>(records.subspan(offset).data());
#else
        const auto *info =
            reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(records.subspan(offset).data());
#endif

        const std::size_t name_end =
            offsetof(FILE_NOTIFY_INFORMATION, FileName) + info->FileNameLength;
        if (offset + name_end > records.size()) {
            log::get()->warn("FileWatcher: truncated change record, ignoring");
            return false;
        }

        try {
            const auto name =
                win32::wchar_to_utf8(static_cast<LPCWCH>(info->FileName),
                                     static_cast<int>(info->FileNameLength / sizeof(WCHAR)));
            if (std::filesystem::path(name) == m_file_name) {
                log::get()->trace("FileWatcher: change detected for {}", name);
                return true;
            }
        } catch (const std::exception &e) {
            log::get()->warn("FileWatcher: wchar_to_utf8 failed: {}", e.what());
            return false;
        }

        if (info->NextEntryOffset == 0) {
            return false;
        }
        offset += info->NextEntryOffset;
    }
    return false;
}

#pragma clang diagnostic pop
