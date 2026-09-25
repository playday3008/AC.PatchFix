#pragma once

#include <memory>
#include <stop_token>

#include <Windows.h>

#include "core/config/file_watcher.hpp"

auto watcher() -> std::unique_ptr<FileWatcher> &;

/// Defined once per plugin by the game's own sources.
void game_init(HMODULE hModule, const std::stop_token &stop);
