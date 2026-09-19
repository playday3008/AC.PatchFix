#include "games/syndicate/hooks/aspect_ratio_fix.hpp"

#include <cstdint>

#include <atomic>
#include <bit>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/syndicate/registry.hpp"
#include "games/syndicate/structs.hpp"

namespace hooks {
    namespace {
        using Tag = games::syndicate::AspectRatioFixHook;

        // Last ratio written, as raw bits, so the hook can log only on a change.
        auto last_aspect_bits() -> std::atomic<std::uint32_t> & {
            static std::atomic<std::uint32_t> instance {0};
            return instance;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        struct MirrorAspect {
            [[maybe_unused]] static constexpr std::string_view name = "AspectRatioFix";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::syndicate::registry().enabled<Tag>()) {
                    return;
                }

                auto *state = reinterpret_cast<games::syndicate::RendererAspectState *>(
                    regs.rbx + games::syndicate::k_renderer_aspect_offset);

                float ratio = games::syndicate::registry().config<Tag>().aspect_ratio.get();
                if (ratio <= 0.0F) {
                    // esi/edi are the width and height the engine was called with. Zero
                    // means no mode is known yet; the engine's own 16:9 default is the
                    // right answer there, so leave every field alone.
                    const auto w = static_cast<std::uint32_t>(regs.rsi);
                    const auto h = static_cast<std::uint32_t>(regs.rdi);
                    if (w == 0 || h == 0) {
                        return;
                    }
                    ratio = static_cast<float>(w) / static_cast<float>(h);
                }

                state->computed.ratio_primary  = ratio;
                state->computed.ratio_fallback = ratio;

                // The consumer reads the active block and picks between the two ratios
                // using its own gate bytes, so both the gates and the values have to
                // follow the block the engine actually computed.
                state->active.gate_primary   = state->computed.gate_primary;
                state->active.gate_odd       = state->computed.gate_odd;
                state->active.gate_secondary = state->computed.gate_secondary;
                state->active.ratio_primary  = ratio;
                state->active.ratio_fallback = ratio;

                // The recompute runs on every mode and window-mode change, so log only
                // when the value actually moves. std::bit_cast keeps this an integer
                // compare; a float one trips -Wfloat-equal and would be wrong for nan.
                const auto bits = std::bit_cast<std::uint32_t>(ratio);
                if (last_aspect_bits().exchange(bits, std::memory_order_relaxed) != bits) {
                    log::get()->info("Syndicate AspectRatioFixHook: aspect {}", ratio);
                }
            }
        };
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        log::get()->trace("Syndicate AspectRatioFixHook: on_reload aspect_ratio={}",
                          cfg.aspect_ratio.get());
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        log::get()->trace("Syndicate AspectRatioFixHook: installing");

        auto addr = addrs.aspect_recompute.value();
        log::get()->trace("Syndicate AspectRatioFixHook: aspect recompute tail at 0x{:X}", addr);

        auto hook_result = mem::make_hook<MirrorAspect>(addr);
        if (!hook_result) {
            log::get()->error("Syndicate AspectRatioFixHook: hook failed: {}", hook_result.error());
            return false;
        }
        g_hook = std::move(*hook_result);

        log::get()->info("Syndicate AspectRatioFixHook: installed");
        return true;
    }
} // namespace hooks
