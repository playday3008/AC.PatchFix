#pragma once

#include <cstdint>
#include <cstring>

#include <optional>

namespace mem::x64 {

    inline auto read_rel(uintptr_t addr, size_t sizeof_operand = 4) -> uintptr_t {
        switch (sizeof_operand) {
            case 1: {
                int8_t rel = 0;
                std::memcpy(&rel, reinterpret_cast<const void *>(addr), 1);
                return addr + 1 + static_cast<uintptr_t>(static_cast<intptr_t>(rel));
            }
            case 2: {
                int16_t rel = 0;
                std::memcpy(&rel, reinterpret_cast<const void *>(addr), 2);
                return addr + 2 + static_cast<uintptr_t>(static_cast<intptr_t>(rel));
            }
            case 4: {
                int32_t rel = 0;
                std::memcpy(&rel, reinterpret_cast<const void *>(addr), 4);
                return addr + 4 + static_cast<uintptr_t>(static_cast<intptr_t>(rel));
            }
            default:
                return 0;
        }
    }

    inline auto branch_target(uintptr_t addr) -> std::optional<uintptr_t> {
        auto opcode = *reinterpret_cast<const uint8_t *>(addr);
        switch (opcode) {
            case 0xE8:
            case 0xE9:
                return read_rel(addr + 1, 4);
            case 0xFF: {
                auto modrm = *reinterpret_cast<const uint8_t *>(addr + 1);
                if (modrm == 0x15 || modrm == 0x25) {
                    auto      ptr_addr = read_rel(addr + 2, 4);
                    uintptr_t target   = 0;
                    std::memcpy(&target, reinterpret_cast<const void *>(ptr_addr), sizeof(target));
                    return target;
                }
                return std::nullopt;
            }
            default:
                return std::nullopt;
        }
    }

} // namespace mem::x64
