#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/game_data.hpp"
#include "games/tags.hpp"

namespace games {
    template<>
    struct game_data<Rogue> {
        static constexpr std::string_view name     = "Rogue";
        static constexpr std::string_view exe_name = "ACC.exe";

        static constexpr float k_default_aspect          = 16.0F / 9.0F;
        static constexpr float k_inv_default_aspect      = 9.0F / 16.0F;
        static constexpr float k_base_width              = 1280.0F;
        static constexpr float k_base_height             = 720.0F;
        static constexpr float k_inv_base_width          = 1.0F / 1280.0F;
        static constexpr float k_inv_base_height         = 1.0F / 720.0F;
        static constexpr float k_fov_base_zoom           = 0.768F;
        static constexpr float k_triple_screen_threshold = 4.0F;
        static constexpr float k_multi_monitor_split     = 0.33333F;
        static constexpr float k_float_tolerance           = 1e-6F;

        struct ResolvedAddresses {
            std::optional<std::uintptr_t> viewport_ratio_load;
            std::optional<std::uintptr_t> viewport_ratio_mul;
            std::optional<std::uintptr_t> scaling_branch_start;
            std::optional<std::uintptr_t> scaling_branch_end;
            std::optional<std::uintptr_t> display_flag;
            std::optional<std::uintptr_t> fov_store;
            std::optional<std::uintptr_t> coord_transform;
            std::optional<std::uintptr_t> scaling_offsets;
            std::optional<std::uintptr_t> game_unpause;
            std::optional<std::uintptr_t> game_pause;
            std::optional<std::uintptr_t> game_pause2;
            std::optional<std::uintptr_t> get_game_id;
            std::optional<std::uintptr_t> lang_bf_write;
            std::optional<std::uintptr_t> lang_setup;
            std::optional<std::uintptr_t> get_language;
            std::optional<std::uintptr_t> fps_sleep_branch;
            std::optional<std::uintptr_t> fps_frame_time;
            std::optional<std::uintptr_t> game_state_global;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="VIEWPORT_RATIO_LOAD", .field=&ResolvedAddresses::viewport_ratio_load,  .offset=0x05, .bytes="0F 28 E8 EB ? F3 0F 10 05 ? ? ? ? F3 0F 5E C1 F3 0F 59 C4"},
            {.name="VIEWPORT_RATIO_MUL",  .field=&ResolvedAddresses::viewport_ratio_mul,   .offset=0x00, .bytes="F3 0F 59 25 ? ? ? ? EB ? 0F 28 E5"},
            {.name="SCALING_BRANCH",      .field=&ResolvedAddresses::scaling_branch_start, .offset=0x00, .bytes="45 0F 2F C1 41 0F 28 F0 41 0F 28 F9 76 ? 41 0F 28 F8 F3 0F 59 3D"},
            {.name="SCALING_BRANCH",      .field=&ResolvedAddresses::scaling_branch_end,   .offset=0x52, .bytes="45 0F 2F C1 41 0F 28 F0 41 0F 28 F9 76 ? 41 0F 28 F8 F3 0F 59 3D"},
            {.name="DISPLAY_FLAG",        .field=&ResolvedAddresses::display_flag,         .offset=0x00, .bytes="0F 2F C8 73 ? 8B D6 F3 0F 59 15"},
            {.name="FOV_STORE",           .field=&ResolvedAddresses::fov_store,            .offset=0x05, .bytes="89 43 40 EB 05 F3 0F 11 73 40 48 8D 54 24 40"},
            {.name="COORD_TRANSFORM",     .field=&ResolvedAddresses::coord_transform,      .offset=0x00, .bytes="0F 28 D0 F3 0F 59 15 ? ? ? ? 0F 2F CA 77"},
            {.name="SCALING_OFFSETS",     .field=&ResolvedAddresses::scaling_offsets,      .offset=0x00, .bytes="F3 0F 11 4C 24 30 F3 0F 59 05 ? ? ? ? F3 0F 11 44 24 34"},
            {.name="GAME_UNPAUSE",        .field=&ResolvedAddresses::game_unpause,         .offset=0x00, .bytes="C6 81 C0 02 00 00 00 48 8B 91 90 02 00 00 48 8B D9"},
            {.name="GAME_PAUSE",          .field=&ResolvedAddresses::game_pause,           .offset=0x11, .bytes="48 C1 E1 20 48 C1 F9 3F 48 23 08 48 39 4A 18 75 08 41 C6 80 C0 02 00 00 01"},
            {.name="GAME_PAUSE2",         .field=&ResolvedAddresses::game_pause2,          .offset=0x06, .bytes="48 39 46 18 75 ? C6 87 C0 02 00 00 01 48 8B 74 24"},
            {.name="GET_GAME_ID",         .field=&ResolvedAddresses::get_game_id,          .offset=0x00, .bytes="48 83 EC 28 B9 ? ? ? ? E8 ? ? ? ? 84 C0 74 ? E8 ? ? ? ? 33 C9 84 C0 0F 95 C1 8D 81 ? ? ? ?"},
            {.name="LANG_BF_WRITE",       .field=&ResolvedAddresses::lang_bf_write,        .offset=0x05, .bytes="0F B6 44 24 ? 89 3D ? ? ? ? 89 1D ? ? ? ? 89 05"},
            {.name="LANG_SETUP",          .field=&ResolvedAddresses::lang_setup,           .offset=0x00, .bytes="8B CB E8 ? ? ? ? E8 ? ? ? ? 8B C8 E8 ? ? ? ?"},
            {.name="GET_LANGUAGE",        .field=&ResolvedAddresses::get_language,         .offset=0x00, .bytes="48 83 EC 28 8B 05 ? ? ? ? 83 F8 17 7C"},
            {.name="FPS_SLEEP_BRANCH",    .field=&ResolvedAddresses::fps_sleep_branch,     .offset=0x0F, .bytes="48 89 43 78 48 2B BB 80 00 00 00 48 3B 43 70 73"},
            {.name="FPS_FRAME_TIME",      .field=&ResolvedAddresses::fps_frame_time,       .offset=0x06, .bytes="79 04 F3 0F 58 C1 F3 0F 59 05"},
            {.name="GAME_STATE_GLOBAL",   .field=&ResolvedAddresses::game_state_global,    .offset=0x00, .bytes="48 8B 05 ? ? ? ? C6 80 C8 02 00 00 00 C3"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<Rogue>);
} // namespace games
