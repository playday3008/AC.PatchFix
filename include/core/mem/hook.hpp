#pragma once

#include <cstddef>
#include <cstdint>

#include <expected>
#include <span>
#include <string>

#include <safetyhook/context.hpp>
#include <safetyhook/mid_hook.hpp>

#include "core/diagnostics/hook_context.hpp"
#include "core/diagnostics/patch_registry.hpp"
#include "core/diagnostics/seh_guard.hpp"
#include "core/mem/write.hpp"

namespace mem {
    using Registers = safetyhook::Context;

    class MidHook {
        safetyhook::MidHook inner_;

      public:
        MidHook()                                             = default;
        ~MidHook()                                            = default;
        MidHook(const MidHook &)                              = delete;
        auto operator=(const MidHook &) -> MidHook &          = delete;
        MidHook(MidHook &&other) noexcept                     = default;
        auto operator=(MidHook &&other) noexcept -> MidHook & = default;

        [[nodiscard]] static auto create(std::uintptr_t addr, void (*callback)(Registers &))
            -> std::expected<MidHook, std::string>;

        explicit operator bool() const { return static_cast<bool>(inner_); }

        void reset() { inner_.reset(); }

        [[nodiscard]] auto original_bytes_size() const -> std::size_t {
            return inner_.original_bytes().size();
        }

        [[nodiscard]] auto original_bytes_data() const -> std::span<const std::uint8_t> {
            const auto &ob = inner_.original_bytes();
            return {ob.data(), ob.size()};
        }
    };

    template<typename Functor>
    [[nodiscard]] auto make_hook(std::uintptr_t addr) -> std::expected<MidHook, std::string> {
        auto result = MidHook::create(addr, diagnostics::guarded_callback<Functor>);
        if (result) {
            diagnostics::patch_registry::register_patch(
                addr,
                result->original_bytes_size(),
                result->original_bytes_data(),
                diagnostics::current_hook_name(),
                diagnostics::patch_registry::PatchType::mid_hook);
        }
        return result;
    }

    template<typename Functor>
    [[nodiscard]] auto make_hook(std::uintptr_t addr, std::uintptr_t end)
        -> std::expected<MidHook, std::string> {
        if (end > addr) {
            (void)nop(addr, end - addr);
        }
        auto result = MidHook::create(addr, diagnostics::guarded_callback<Functor>);
        if (result) {
            diagnostics::patch_registry::register_patch(
                addr,
                result->original_bytes_size(),
                result->original_bytes_data(),
                diagnostics::current_hook_name(),
                diagnostics::patch_registry::PatchType::mid_hook);
        }
        return result;
    }
} // namespace mem
