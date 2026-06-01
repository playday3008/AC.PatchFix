#include "games/rogue/hooks/viewport_fitting.hpp"

#include <atomic>
#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"
#include "games/rogue/structs.hpp"

namespace hooks {
    auto current_aspect() -> std::atomic<float> & {
        static std::atomic<float> instance {games::game_data<games::Rogue>::k_default_aspect};
        return instance;
    }

    namespace {
        using Data = games::game_data<games::Rogue>;
        using Tag  = games::rogue::ViewportFittingHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_ratio_load_hook;
        mem::MidHook g_ratio_mul_hook;
        mem::MidHook g_coord_hook;
#pragma clang diagnostic pop

        struct ViewportRatioLoad {
            static constexpr std::string_view name = "ViewportFitting/RatioLoad";
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::rogue::registry().enabled<Tag>() ||
                    !is_in_game().load(std::memory_order_relaxed)) {
                    regs.xmm0.f32[0] = Data::k_inv_default_aspect;
                    return;
                }
                const float ar = games::rogue::registry().config<Tag>().aspect_ratio.get();
                if (ar > 0.0F) {
                    regs.xmm0.f32[0] = 1.0F / ar;
                    current_aspect().store(ar, std::memory_order_relaxed);
                    return;
                }
                const auto *display =
                    reinterpret_cast<const games::rogue::DisplaySettings *>(regs.rax);
                const float w = display->width;
                const float h = display->height;
                if (w > 0.0F) {
                    regs.xmm0.f32[0] = h / w;
                    current_aspect().store(w / h, std::memory_order_relaxed);
                }
            }
        };

        struct ViewportRatioMul {
            static constexpr std::string_view name = "ViewportFitting/RatioMul";
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::rogue::registry().enabled<Tag>() ||
                    !is_in_game().load(std::memory_order_relaxed)) {
                    regs.xmm4.f32[0] *= Data::k_default_aspect;
                    return;
                }
                const float ar = games::rogue::registry().config<Tag>().aspect_ratio.get();
                if (ar > 0.0F) {
                    regs.xmm4.f32[0] *= ar;
                    return;
                }
                const auto *display =
                    reinterpret_cast<const games::rogue::DisplaySettings *>(regs.rax);
                const float w = display->width;
                const float h = display->height;
                if (h > 0.0F) {
                    regs.xmm4.f32[0] *= (w / h);
                }
            }
        };

        struct CoordTransformHook {
            static constexpr std::string_view name = "ViewportFitting/CoordTransform";
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto *a5_x = reinterpret_cast<float *>(regs.r10);
                auto *a5_y = reinterpret_cast<float *>(regs.r10 + 4);

                if (games::rogue::registry().enabled<Tag>()) {
                    return;
                }

                const float w        = regs.xmm0.f32[0];
                const float h        = regs.xmm1.f32[0];
                const float fitted_h = w * Data::k_inv_default_aspect;
                if (h > fitted_h) {
                    *a5_y = regs.xmm3.f32[0] * h / fitted_h;
                } else {
                    *a5_x = (w * *a5_x) / (h * Data::k_default_aspect);
                }
            }
        };
    } // namespace

    void HookTraits<games::rogue::ViewportFittingHook>::on_reload(const Config &cfg) {
        float ar = cfg.aspect_ratio.get();
        log::get()->trace("ViewportFittingHook: on_reload aspect_ratio={}", ar);
        if (ar > 0.0F) {
            current_aspect().store(ar, std::memory_order_relaxed);
        }
    }

    auto HookTraits<games::rogue::ViewportFittingHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("ViewportFittingHook: installing");
        auto ratio_load = addrs.viewport_ratio_load.value();
        auto ratio_mul  = addrs.viewport_ratio_mul.value();
        if (auto h = mem::make_hook<ViewportRatioLoad>(ratio_load, ratio_load + 8)) {
            g_ratio_load_hook = std::move(*h);
        } else {
            log::get()->error("ViewportFittingHook: ratio_load hook failed: {}", h.error());
            return false;
        }
        if (auto h = mem::make_hook<ViewportRatioMul>(ratio_mul, ratio_mul + 8)) {
            g_ratio_mul_hook = std::move(*h);
        } else {
            log::get()->error("ViewportFittingHook: ratio_mul hook failed: {}", h.error());
            return false;
        }
        if (addrs.coord_transform) {
            auto ct = addrs.coord_transform.value();
            if (auto h = mem::make_hook<CoordTransformHook>(ct, ct + 38)) {
                g_coord_hook = std::move(*h);
            } else {
                log::get()->error("ViewportFittingHook: coord_transform hook failed: {}",
                                  h.error());
                return false;
            }
            log::get()->trace("ViewportFittingHook: coord_transform at 0x{:X}", ct);
        }
        log::get()->trace("ViewportFittingHook: installed");
        return true;
    }
} // namespace hooks
