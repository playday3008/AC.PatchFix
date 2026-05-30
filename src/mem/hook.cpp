#include "mem/hook.hpp"

#include <cstdint>

#include <expected>
#include <string>
#include <utility>

#include <safetyhook/mid_hook.hpp>

namespace mem {
    auto MidHook::create(uintptr_t addr, void (*callback)(Registers &))
        -> std::expected<MidHook, std::string> {
        auto result = safetyhook::MidHook::create(reinterpret_cast<void *>(addr), callback);
        if (!result) {
            auto &err = result.error();
            switch (err.type) {
                case safetyhook::MidHook::Error::BAD_ALLOCATION:
                    return std::unexpected(std::string("MidHook: allocation failed"));
                case safetyhook::MidHook::Error::BAD_INLINE_HOOK:
                    return std::unexpected(std::string("MidHook: inline hook failed"));
                default:
                    return std::unexpected(std::string("MidHook: unknown error"));
            }
        }
        MidHook hook;
        hook.inner_ = std::move(*result);
        return hook;
    }
} // namespace mem
