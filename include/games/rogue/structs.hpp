#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

namespace games::rogue {
    struct DisplaySettings {
        std::uintptr_t           vtable;             // +0x00
        std::uint32_t            origin_x;           // +0x08
        std::uint32_t            origin_y;           // +0x0C
        float                    width;              // +0x10
        float                    height;             // +0x14
        std::uint8_t             multi_monitor_flag; // +0x18
        std::array<std::byte, 3> pad_19;             // +0x19
        std::uint32_t            single_width;       // +0x1C
    };
    static_assert(0x00 == offsetof(DisplaySettings, vtable));
    static_assert(0x08 == offsetof(DisplaySettings, origin_x));
    static_assert(0x10 == offsetof(DisplaySettings, width));
    static_assert(0x14 == offsetof(DisplaySettings, height));
    static_assert(0x18 == offsetof(DisplaySettings, multi_monitor_flag));
    static_assert(0x1C == offsetof(DisplaySettings, single_width));
    static_assert(0x20 == sizeof(DisplaySettings));

    struct CameraSettings {
        std::array<float, 16> transform; // +0x00  4x4 matrix
        float                 fov;       // +0x40
    };
    static_assert(0x40 == offsetof(CameraSettings, fov));
    static_assert(0x44 == sizeof(CameraSettings));

    // Frame pacing state from UpdateFrameTiming (0x140821150)
    struct FrameTiming {
        std::uint32_t         stability_counter; // +0x00
        std::int32_t          current_step;      // +0x04
        std::int32_t          num_steps;         // +0x08
        std::array<float, 10> step_thresholds;   // +0x0C
        std::array<float, 15> avg_samples;       // +0x34  ring buffer
        std::uint64_t         target_time;       // +0x70  perf counter
        std::uint64_t         current_time;      // +0x78  perf counter
        std::uint64_t         previous_time;     // +0x80  perf counter
        std::uint64_t         accumulated_ticks; // +0x88
        float                 fixed_rate;        // +0x90
        std::uint32_t         timing_mode;       // +0x94  0=fixed 1=adaptive 2=vsync 3=averaged
        std::uint8_t          ring_index;        // +0x98  wraps at 15
    };
    static_assert(0x00 == offsetof(FrameTiming, stability_counter));
    static_assert(0x04 == offsetof(FrameTiming, current_step));
    static_assert(0x08 == offsetof(FrameTiming, num_steps));
    static_assert(0x0C == offsetof(FrameTiming, step_thresholds));
    static_assert(0x34 == offsetof(FrameTiming, avg_samples));
    static_assert(0x70 == offsetof(FrameTiming, target_time));
    static_assert(0x78 == offsetof(FrameTiming, current_time));
    static_assert(0x80 == offsetof(FrameTiming, previous_time));
    static_assert(0x88 == offsetof(FrameTiming, accumulated_ticks));
    static_assert(0x90 == offsetof(FrameTiming, fixed_rate));
    static_assert(0x94 == offsetof(FrameTiming, timing_mode));
    static_assert(0x98 == offsetof(FrameTiming, ring_index));
    static_assert(0xA0 == sizeof(FrameTiming));

    // Global: g_pGameState at 0x14329ED60
    // Constructor: sub_1402AE1C0  Destructor: sub_1402A5660
    struct GameState {
        std::uintptr_t              vtable;          // +0x000
        std::array<std::byte, 0x08> pad_008;         // +0x008
        std::array<std::byte, 0x10> buffer_1;        // +0x010
        std::array<std::byte, 0x10> buffer_2;        // +0x020
        std::uintptr_t              context_ptr_1;   // +0x030
        std::uintptr_t              context_ptr_2;   // +0x038
        std::uintptr_t              context_ptr_3;   // +0x040
        std::uintptr_t              context_ptr_4;   // +0x048
        std::array<std::byte, 0xF0> list_nodes;      // +0x050  6x IntrusiveListNode (40B each)
        std::array<std::byte, 0x08> link_data;       // +0x140
        std::uintptr_t              node_vtable;     // +0x148
        std::uintptr_t              self_ptr;        // +0x150
        std::uintptr_t              callback_ptr;    // +0x158
        std::array<std::byte, 0x20> scheduler;       // +0x160
        std::uintptr_t              iface_vtable_1;  // +0x180
        std::array<std::byte, 0x10> iface_data_1;    // +0x188
        std::uintptr_t              iface_vtable_2;  // +0x198
        std::array<std::byte, 0x10> iface_data_2;    // +0x1A0
        std::array<std::byte, 0xB0> event_system;    // +0x1B0
        std::uintptr_t              scene_context;   // +0x260
        std::array<std::byte, 0x20> pad_268;         // +0x268
        std::uint8_t                is_ready;        // +0x288
        std::uint8_t                is_initialized;  // +0x289
        std::uint8_t                pad_28a;         // +0x28A
        std::uint8_t                update_flag;     // +0x28B
        std::array<std::byte, 0x04> pad_28c;         // +0x28C
        std::uintptr_t              world_context;   // +0x290
        std::uintptr_t              session_ref;     // +0x298
        std::uintptr_t              session_handler; // +0x2A0
        std::int32_t                state_index;     // +0x2A8
        std::array<std::byte, 0x04> pad_2ac;         // +0x2AC
        std::uintptr_t              pending_session; // +0x2B0
        std::uint8_t                transition_flag; // +0x2B8
        std::array<std::byte, 0x03> pad_2b9;         // +0x2B9
        std::uint32_t               load_state;      // +0x2BC
        std::uint8_t                pause_flag;      // +0x2C0
        std::array<std::byte, 0x03> pad_2c1;         // +0x2C1
        std::uint32_t               pause_mode;      // +0x2C4
        std::uint8_t                cleanup_flag;    // +0x2C8
        std::array<std::byte, 0x07> pad_2c9;         // +0x2C9
        std::uintptr_t              save_ref;        // +0x2D0
        std::uintptr_t              checkpoint_ref;  // +0x2D8
    };
    static_assert(0x000 == offsetof(GameState, vtable));
    static_assert(0x010 == offsetof(GameState, buffer_1));
    static_assert(0x020 == offsetof(GameState, buffer_2));
    static_assert(0x030 == offsetof(GameState, context_ptr_1));
    static_assert(0x050 == offsetof(GameState, list_nodes));
    static_assert(0x148 == offsetof(GameState, node_vtable));
    static_assert(0x150 == offsetof(GameState, self_ptr));
    static_assert(0x158 == offsetof(GameState, callback_ptr));
    static_assert(0x160 == offsetof(GameState, scheduler));
    static_assert(0x180 == offsetof(GameState, iface_vtable_1));
    static_assert(0x198 == offsetof(GameState, iface_vtable_2));
    static_assert(0x260 == offsetof(GameState, scene_context));
    static_assert(0x288 == offsetof(GameState, is_ready));
    static_assert(0x289 == offsetof(GameState, is_initialized));
    static_assert(0x28B == offsetof(GameState, update_flag));
    static_assert(0x290 == offsetof(GameState, world_context));
    static_assert(0x298 == offsetof(GameState, session_ref));
    static_assert(0x2A0 == offsetof(GameState, session_handler));
    static_assert(0x2A8 == offsetof(GameState, state_index));
    static_assert(0x2B0 == offsetof(GameState, pending_session));
    static_assert(0x2B8 == offsetof(GameState, transition_flag));
    static_assert(0x2BC == offsetof(GameState, load_state));
    static_assert(0x2C0 == offsetof(GameState, pause_flag));
    static_assert(0x2C4 == offsetof(GameState, pause_mode));
    static_assert(0x2C8 == offsetof(GameState, cleanup_flag));
    static_assert(0x2D0 == offsetof(GameState, save_ref));
    static_assert(0x2D8 == offsetof(GameState, checkpoint_ref));
    static_assert(0x2E0 == sizeof(GameState));
} // namespace games::rogue
