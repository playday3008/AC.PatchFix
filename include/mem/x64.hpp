#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <optional>

namespace mem::x64 {
    inline constexpr uint8_t op_call_rel32  = 0xE8;
    inline constexpr uint8_t op_jmp_rel32   = 0xE9;
    inline constexpr uint8_t op_indirect    = 0xFF;
    inline constexpr uint8_t modrm_call_rip = 0x15;
    inline constexpr uint8_t modrm_jmp_rip  = 0x25;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"

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
            case op_call_rel32:
            case op_jmp_rel32:
                return read_rel(addr + 1, 4);
            case op_indirect: {
                auto modrm = *reinterpret_cast<const uint8_t *>(addr + 1);
                if (modrm == modrm_call_rip || modrm == modrm_jmp_rip) {
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

#pragma clang diagnostic pop
} // namespace mem::x64
