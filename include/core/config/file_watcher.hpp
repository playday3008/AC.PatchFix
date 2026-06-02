#pragma once

#include <filesystem>
#include <functional>
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

  private:
    void watch_loop(const std::stop_token &token);

    std::filesystem::path m_dir_path;
    std::filesystem::path m_file_name;
    Callback              m_on_change;
    std::jthread          m_thread;
};
