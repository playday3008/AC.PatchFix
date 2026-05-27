#include "hooks/common/viewport_scaling.hpp"

#include <atomic>

#include <injector/assembly.hpp>
#include <injector/injector.hpp>

#include "logger.hpp" // IWYU pragma: keep

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"

namespace hooks {
    namespace {
        using G    = games::Rogue;
        using Data = games::game_data<G>;
        using Tag  = ViewportScalingHook<G>;

        std::atomic<float> g_active_stretch {0.0F};

        struct ViewportScalingBranch {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                float w = regs.xmm8.f32[0];
                float h = regs.xmm9.f32[0];

                if (!games::rogue::g_registry.enabled<Tag>()) {
                    g_active_stretch.store(0.0F, std::memory_order_relaxed);
                    if (w > h) {
                        float fitted_h   = w * Data::k_inv_default_aspect;
                        regs.xmm6.f32[0] = w;
                        regs.xmm7.f32[0] = (h < fitted_h) ? h : fitted_h;
                        regs.xmm4.f32[0] = regs.xmm7.f32[0] * Data::k_inv_base_height;
                    } else {
                        float fitted_w   = h * Data::k_default_aspect;
                        regs.xmm7.f32[0] = h;
                        regs.xmm6.f32[0] = (w < fitted_w) ? w : fitted_w;
                        regs.xmm4.f32[0] = regs.xmm6.f32[0] * Data::k_inv_base_width;
                    }
                    return;
                }

                float scale_w = w * Data::k_inv_base_width;
                float scale_h = h * Data::k_inv_base_height;

                const auto &cfg = games::rogue::g_registry.config<Tag>();

                if (scale_w <= scale_h) {
                    float stretch = cfg.ui_stretch_v.get();
                    g_active_stretch.store(stretch, std::memory_order_relaxed);
                    float fitted_h   = w * Data::k_inv_default_aspect;
                    float clamped_h  = (h < fitted_h) ? h : fitted_h;
                    regs.xmm6.f32[0] = w;
                    regs.xmm7.f32[0] = clamped_h + (stretch * (h - clamped_h));
                    regs.xmm4.f32[0] = regs.xmm7.f32[0] * Data::k_inv_base_height;
                } else {
                    float stretch = cfg.ui_stretch_h.get();
                    g_active_stretch.store(stretch, std::memory_order_relaxed);
                    float fitted_w   = h * Data::k_default_aspect;
                    float clamped_w  = (w < fitted_w) ? w : fitted_w;
                    regs.xmm7.f32[0] = h;
                    regs.xmm6.f32[0] = clamped_w + (stretch * (w - clamped_w));
                    regs.xmm4.f32[0] = regs.xmm6.f32[0] * Data::k_inv_base_width;
                }
            }
        };

        struct ScalingOffsetsHook {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                float fade = 1.0F - g_active_stretch.load(std::memory_order_relaxed);

                float offset_x = regs.xmm1.f32[0] * fade;
                float offset_y = regs.xmm0.f32[0] * 0.5F * fade;

                *reinterpret_cast<float *>(regs.rsp + 0x30) = offset_x;
                *reinterpret_cast<float *>(regs.rsp + 0x34) = offset_y;
            }
        };
    } // namespace

    template<>
    auto HookTraits<ViewportScalingHook<games::Rogue>>::install(const Addrs &addrs) -> bool {
        log::get()->trace("ViewportScalingHook: installing");
        auto start = addrs.scaling_branch_start.value();
        auto end   = addrs.scaling_branch_end.value();
        if (end <= start) {
            log::get()->trace("ViewportScalingHook: invalid range 0x{:X}-0x{:X}", start, end);
            return false;
        }
        injector::MakeInline<ViewportScalingBranch>(start, end);
        if (addrs.scaling_offsets) {
            auto offsets = addrs.scaling_offsets.value();
            injector::MakeInline<ScalingOffsetsHook>(offsets, offsets + 20);
            log::get()->trace("ViewportScalingHook: scaling_offsets at 0x{:X}", offsets);
        }
        log::get()->trace("ViewportScalingHook: installed");
        return true;
    }
} // namespace hooks
