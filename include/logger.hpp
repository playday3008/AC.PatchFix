#pragma once

#include <cstddef>

#include <memory>
#include <source_location>
#include <string_view>

#include <spdlog/spdlog.h>

namespace detail {
    consteval auto extract_logger_name(const char *filepath) -> std::string_view {
        std::string_view path(filepath);

        auto        last_sep   = path.find_last_of("/\\");
        std::size_t stem_begin = (last_sep != std::string_view::npos) ? last_sep + 1 : 0;

        auto        dot      = path.rfind('.');
        std::size_t stem_end = (dot != std::string_view::npos && dot >= stem_begin) ? dot
                                                                                    : path.size();

        if (last_sep != std::string_view::npos && last_sep > 0) {
            auto        prev_sep     = path.find_last_of("/\\", last_sep - 1);
            std::size_t parent_begin = (prev_sep != std::string_view::npos) ? prev_sep + 1 : 0;
            return path.substr(parent_begin, stem_end - parent_begin);
        }

        return path.substr(stem_begin, stem_end - stem_begin);
    }
} // namespace detail

class log {
  public:
    static void init(std::string_view path,
                     std::size_t      maxSize  = 1ULL * 1024 * 1024,
                     std::size_t      maxFiles = 3ULL);

    static void shutdown();

    static auto get(std::string_view name =
                        detail::extract_logger_name(std::source_location::current().file_name()))
        -> std::shared_ptr<spdlog::logger>;

    log(const log &)                     = delete;
    log(log &&)                          = delete;
    auto operator=(const log &) -> log & = delete;
    auto operator=(log &&) -> log &      = delete;

    ~log();

  private:
    log(std::string_view path, std::size_t maxSize, std::size_t maxFiles);

    struct Impl;
    std::unique_ptr<Impl> m;
};
