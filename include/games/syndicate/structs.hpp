#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

#include <guiddef.h>

namespace games::syndicate {
    // Display mode entry passed to ModeList_InsertSorted (0x141C72310)
    struct ModeEntry {
        std::uintptr_t vtable;       // +0x00
        std::uint32_t  width;        // +0x08
        std::uint32_t  height;       // +0x0C
        std::uint32_t  refresh_rate; // +0x10
        std::uint32_t  format;       // +0x14
    };
    static_assert(0x08 == offsetof(ModeEntry, width));
    static_assert(0x0C == offsetof(ModeEntry, height));
    static_assert(0x10 == offsetof(ModeEntry, refresh_rate));
    static_assert(0x14 == offsetof(ModeEntry, format));
    static_assert(0x18 == sizeof(ModeEntry));

    // Per-device entry in DeviceManager::slot_array (568 bytes each)
    // Populated by register_device_slot (sub_1423219B0)
    // Packed: bindings_ptr at +0x22C is not 8-byte aligned
#pragma pack(push, 1)
    struct DeviceSlot {
        void                    *device_obj;     // +0x000  vtable-based device driver object
        std::uint32_t            device_type;    // +0x008  0=keyboard 2=xbox 4=ps3 5=ds4
        std::array<wchar_t, 260> name;           // +0x00C  device display name
        std::uint16_t            vendor_id;      // +0x214
        std::uint16_t            product_id;     // +0x216
        std::int32_t             xbox_subtype;   // +0x218  -1 if not xbox
        GUID                     guid;           // +0x21C  DirectInput device GUID
        void                    *bindings_ptr;   // +0x22C  action-map binding array
        std::uint16_t            bindings_cap;   // +0x234
        std::uint16_t            bindings_count; // +0x236
    };
#pragma pack(pop)
    static_assert(0x008 == offsetof(DeviceSlot, device_type));
    static_assert(0x00C == offsetof(DeviceSlot, name));
    static_assert(0x214 == offsetof(DeviceSlot, vendor_id));
    static_assert(0x216 == offsetof(DeviceSlot, product_id));
    static_assert(0x218 == offsetof(DeviceSlot, xbox_subtype));
    static_assert(0x21C == offsetof(DeviceSlot, guid));
    static_assert(0x22C == offsetof(DeviceSlot, bindings_ptr));
    static_assert(0x236 == offsetof(DeviceSlot, bindings_count));
    static_assert(0x238 == sizeof(DeviceSlot));

    // Input device manager — holds the device slot ring buffer
    // Accessed via InputSystemState::device_manager (sub_14234F870 tick fn)
    // Not fully defined: 0x7C0+ bytes with misaligned pointer at +0x79C
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    struct DeviceManager {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        [[nodiscard]] auto active_device_index() const -> std::uint32_t {
            return *reinterpret_cast<const std::uint32_t *>(reinterpret_cast<const char *>(this) +
                                                            0x798);
        }

        [[nodiscard]] auto slot_array() const -> const DeviceSlot * {
            return *reinterpret_cast<const DeviceSlot *const *>(
                reinterpret_cast<const char *>(this) + 0x79C);
        }

        [[nodiscard]] auto active_slot() const -> const DeviceSlot & {
            return slot_array()[active_device_index()];
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    };
#pragma clang diagnostic pop

    // Intermediate object between InputContext and DeviceManager
    struct InputSystemState {
        void          *_field_0;       // +0x00
        DeviceManager *device_manager; // +0x08
    };
    static_assert(0x08 == offsetof(InputSystemState, device_manager));

    // Top-level input singleton at qword_147333CB8
    // Passed as rcx to get_active_device_type (sub_142331CA0)
    struct InputContext {
        void                    *vtable; // +0x00
        std::array<std::byte, 8> _pad08; // +0x08
        InputSystemState        *state;  // +0x10
    };
    static_assert(0x10 == offsetof(InputContext, state));
} // namespace games::syndicate
