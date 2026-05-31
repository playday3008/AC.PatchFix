#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace games::rogue {
    struct DisplaySettings {
        std::array<std::byte, 0x08> pad_00;
        std::uint32_t               origin_x;           // +0x08
        std::uint32_t               origin_y;           // +0x0C
        float                       width;              // +0x10
        float                       height;             // +0x14
        std::uint8_t                multi_monitor_flag; // +0x18
        std::array<std::byte, 3>    pad_19;
        std::uint32_t               single_width; // +0x1C
    };
    static_assert(offsetof(DisplaySettings, width) == 0x10);
    static_assert(offsetof(DisplaySettings, height) == 0x14);
    static_assert(offsetof(DisplaySettings, multi_monitor_flag) == 0x18);
    static_assert(offsetof(DisplaySettings, single_width) == 0x1C);
    static_assert(sizeof(DisplaySettings) == 0x20);

    struct CameraSettings {
        std::array<float, 16> transform; // +0x00  4x4 matrix
        float                 fov;       // +0x40
    };
    static_assert(offsetof(CameraSettings, fov) == 0x40);
    static_assert(sizeof(CameraSettings) == 0x44);

    struct GameState {
        std::array<std::byte, 0x2C0> pad_00;
        std::uint8_t                 pause_flag; // +0x2C0
    };
    static_assert(offsetof(GameState, pause_flag) == 0x2C0);
    static_assert(sizeof(GameState) == 0x2C1);
} // namespace games::rogue
