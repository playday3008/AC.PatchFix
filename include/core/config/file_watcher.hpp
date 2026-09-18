#pragma once

#include <cstddef>

#include <atomic>
#include <filesystem>
#include <functional>
#include <span>
#include <stop_token>
#include <thread>

class FileWatcher {
  public:
    using Callback = std::move_only_function<void()>;

    FileWatcher(const std::filesystem::path &file_path, Callback on_change);
    ~FileWatcher() = default;

    FileWatcher(const FileWatcher &)                     = delete;
    FileWatcher(FileWatcher &&)                          = delete;
    auto operator=(const FileWatcher &) -> FileWatcher & = delete;
    auto operator=(FileWatcher &&) -> FileWatcher &      = delete;

    // Ask the watch loop to exit without waiting for it. Joining is not safe from
    // DllMain, where the loader lock the watch loop may need is already held, so
    // the caller there leaks the object instead of destroying it. The loop stops
    // invoking the callback as soon as m_stopping is observed.
    void stop();

  private:
    void watch_loop(const std::stop_token &token);

    [[nodiscard]] auto records_name_our_file(std::span<const std::byte> records) const -> bool;

    std::filesystem::path m_dir_path;
    std::filesystem::path m_file_name;
    Callback              m_on_change;
    std::atomic<bool>     m_stopping {false};
    std::jthread          m_thread;
};
