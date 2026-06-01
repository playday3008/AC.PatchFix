#include "games/rogue/hooks/fov_correction.hpp"

#include <cmath>

#include <atomic>
#include <string_view>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep

#include "mem/hook.hpp"

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"
#include "games/rogue/structs.hpp"

auto games::rogue::compute_hor_plus_correction() -> float {
    using Data = games::game_data<games::Rogue>;

    const float aspect = hooks::current_aspect().load(std::memory_order_relaxed);
    if (aspect <= 0.0F || std::abs(aspect - Data::k_default_aspect) < Data::k_float_epsilon) {
        return 1.0F;
    }

    const float ratio     = Data::k_default_aspect / aspect;
    const float corrected = 2.0F * std::atan(Data::k_fov_base_zoom * ratio);
    const float original  = 2.0F * std::atan(Data::k_fov_base_zoom);
    if (std::abs(original) < Data::k_float_epsilon) {
        return 1.0F;
    }
    return corrected / original;
}

namespace hooks {
    using games::rogue::FovMode;

    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_fov_hook;
#pragma clang diagnostic pop

        using Data = games::game_data<games::Rogue>;
        using Tag  = games::rogue::FOVCorrectionHook;

        struct FOVCorrectionFunctor {
            static constexpr std::string_view name = "FOVCorrection";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto *camera = reinterpret_cast<games::rogue::CameraSettings *>(regs.rbx);

                if (!games::rogue::registry().enabled<Tag>()) {
                    camera->fov = regs.xmm6.f32[0];
                    return;
                }

                float fov = regs.xmm6.f32[0];

                const auto &cfg            = games::rogue::registry().config<Tag>();
                auto        mode           = cfg.mode.get();
                bool        apply_hor_plus = false;
                switch (mode) {
                    case FovMode::Auto:
                        apply_hor_plus = (current_aspect().load(std::memory_order_relaxed) <
                                          Data::k_default_aspect);
                        break;
                    case FovMode::VertPlus:
                        break;
                    case FovMode::HorPlus:
                        apply_hor_plus = true;
                        break;
                    case FovMode::_count:
                    default:
                        std::unreachable();
                }
                if (apply_hor_plus) {
                    fov *= games::rogue::compute_hor_plus_correction();
                }

                fov *= cfg.multiplier.get();

                camera->fov = fov;
            }
        };
    } // namespace

    auto HookTraits<games::rogue::FOVCorrectionHook>::install(const Addrs &addrs) -> bool {
        log::get()->trace("FOVCorrectionHook: installing at 0x{:X}", addrs.fov_store.value());
        auto addr = addrs.fov_store.value();
        if (auto h = mem::make_hook<FOVCorrectionFunctor>(addr, addr + 5)) {
            g_fov_hook = std::move(*h);
        } else {
            log::get()->error("FOVCorrectionHook: hook failed: {}", h.error());
            return false;
        }
        log::get()->trace("FOVCorrectionHook: installed");
        return true;
    }
} // namespace hooks
