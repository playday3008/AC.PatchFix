#pragma once

#include <cstdint>

#include <expected>
#include <string>
#include <utility>

#include <safetyhook/context.hpp>
#include <safetyhook/mid_hook.hpp>

#include "mem/write.hpp"

namespace mem {

    using Registers = safetyhook::Context;

    class MidHook {
        safetyhook::MidHook inner_;

      public:
        MidHook()                                             = default;
        MidHook(MidHook &&other) noexcept                     = default;
        auto operator=(MidHook &&other) noexcept -> MidHook & = default;

        static auto create(uintptr_t addr, void (*callback)(Registers &))
            -> std::expected<MidHook, std::string>;

        explicit operator bool() const { return static_cast<bool>(inner_); }

        void reset() { inner_.reset(); }
    };

    template<typename Functor>
    auto make_hook(uintptr_t addr) -> std::expected<MidHook, std::string> {
        return MidHook::create(addr, [](Registers &regs) { Functor {}(regs); });
    }

    template<typename Functor>
    auto make_hook(uintptr_t addr, uintptr_t end) -> std::expected<MidHook, std::string> {
        if (end > addr) {
            nop(addr, end - addr);
        }
        return MidHook::create(addr, [](Registers &regs) { Functor {}(regs); });
    }

} // namespace mem
