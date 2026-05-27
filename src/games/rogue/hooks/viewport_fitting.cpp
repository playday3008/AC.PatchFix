#include "hooks/common/viewport_fitting.hpp"

#include <injector/assembly.hpp>
#include <injector/injector.hpp>

#include "logger.hpp" // IWYU pragma: keep

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"
#include "hooks/common/game_state.hpp"

namespace hooks {
    namespace {
        using G    = games::Rogue;
        using Data = games::game_data<G>;
        using Tag  = ViewportFittingHook<G>;

        struct ViewportRatioLoad {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                if (!games::rogue::g_registry.enabled<Tag>() ||
                    !g_is_in_game<G>.load(std::memory_order_relaxed)) {
                    regs.xmm0.f32[0] = Data::k_inv_default_aspect;
                    return;
                }
                float ar = games::rogue::g_registry.config<Tag>().aspect_ratio.get();
                if (ar > 0.0F) {
                    regs.xmm0.f32[0] = 1.0F / ar;
                    g_current_aspect<G>.store(ar, std::memory_order_relaxed);
                    return;
                }
                float w = *reinterpret_cast<float *>(regs.rax + 0x10);
                float h = *reinterpret_cast<float *>(regs.rax + 0x14);
                if (w > 0.0F) {
                    regs.xmm0.f32[0] = h / w;
                    g_current_aspect<G>.store(w / h, std::memory_order_relaxed);
                }
            }
        };

        struct ViewportRatioMul {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                if (!games::rogue::g_registry.enabled<Tag>() ||
                    !g_is_in_game<G>.load(std::memory_order_relaxed)) {
                    regs.xmm4.f32[0] *= Data::k_default_aspect;
                    return;
                }
                float ar = games::rogue::g_registry.config<Tag>().aspect_ratio.get();
                if (ar > 0.0F) {
                    regs.xmm4.f32[0] *= ar;
                    return;
                }
                float w = *reinterpret_cast<float *>(regs.rax + 0x10);
                float h = *reinterpret_cast<float *>(regs.rax + 0x14);
                if (h > 0.0F) {
                    regs.xmm4.f32[0] *= (w / h);
                }
            }
        };

        struct CoordTransformHook {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                auto *a5_x = reinterpret_cast<float *>(regs.r10);
                auto *a5_y = reinterpret_cast<float *>(regs.r10 + 4);

                if (games::rogue::g_registry.enabled<Tag>()) {
                    return;
                }

                float w        = regs.xmm0.f32[0];
                float h        = regs.xmm1.f32[0];
                float fitted_h = w * Data::k_inv_default_aspect;
                if (h > fitted_h) {
                    *a5_y = regs.xmm3.f32[0] * h / fitted_h;
                } else {
                    *a5_x = (w * *a5_x) / (h * Data::k_default_aspect);
                }
            }
        };
    } // namespace

    template<>
    void HookTraits<ViewportFittingHook<games::Rogue>>::on_reload(const Config &cfg) {
        float ar = cfg.aspect_ratio.get();
        log::get()->trace("ViewportFittingHook: on_reload aspect_ratio={}", ar);
        if (ar > 0.0F) {
            g_current_aspect<G>.store(ar, std::memory_order_relaxed);
        }
    }

    template<>
    auto HookTraits<ViewportFittingHook<games::Rogue>>::install(const Addrs &addrs) -> bool {
        log::get()->trace("ViewportFittingHook: installing");
        auto ratio_load = addrs.viewport_ratio_load.value();
        auto ratio_mul  = addrs.viewport_ratio_mul.value();
        injector::MakeInline<ViewportRatioLoad>(ratio_load, ratio_load + 8);
        injector::MakeInline<ViewportRatioMul>(ratio_mul, ratio_mul + 8);
        if (addrs.coord_transform) {
            auto ct = addrs.coord_transform.value();
            injector::MakeInline<CoordTransformHook>(ct, ct + 38);
            log::get()->trace("ViewportFittingHook: coord_transform at 0x{:X}", ct);
        }
        log::get()->trace("ViewportFittingHook: installed");
        return true;
    }
} // namespace hooks
