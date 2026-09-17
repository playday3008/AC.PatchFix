#include "games/rogue/hooks/camera_lean.hpp"

#include <cstdint>

#include <algorithm>
#include <atomic>
#include <string_view>
#include <utility>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/mem/write.hpp"
#include "core/mem/x64.hpp"

#include "games/rogue/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::rogue::CameraLeanHook;

        // The sprint lean is a sequence track bound to the roll angle of the
        // CameraFXInterface object the camera manager owns. The track uses the
        // engine's additive blend, which adds curve * weight to the current value
        // once per frame with no delta time, so the lean climbs faster the more
        // frames are drawn: a gentle sway at 60 fps becomes a 30-40 degree roll at
        // 200. Scaling the weight by the measured frame time restores the rate the
        // curve was tuned for.

        // Camera manager -> CameraFXInterface.
        constexpr std::uintptr_t k_camera_fx_slot = 0x38;

        // Track binding layout at the dispatcher: [rbx+8] is the descriptor, whose
        // low three flag bits select the blend function; [rbx+264] is an array of
        // 56-byte entries (count at +274) whose +40 is the property owner.
        constexpr std::uintptr_t k_binding_descriptor = 0x08;
        constexpr std::uintptr_t k_binding_entries    = 264;
        constexpr std::uintptr_t k_binding_count      = 274;
        constexpr std::uintptr_t k_entry_size         = 56;
        constexpr std::uintptr_t k_entry_target       = 40;
        constexpr std::uint32_t  k_mode_mask          = 7;
        constexpr std::uint32_t  k_mode_additive      = 1;

        // Frame interval smoothing. A hitch would otherwise scale one frame's
        // increment by a huge factor; the correction wants the sustained rate.
        constexpr float k_dt_alpha         = 0.05F;
        constexpr float k_max_plausible_dt = 0.2F;
        constexpr float k_max_scale        = 4.0F;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_frame_hook;
        mem::MidHook g_weight_hook;

        std::uintptr_t             g_camera_manager = 0;
        std::atomic<float>         g_reference_fps {30.0F};
        std::atomic<float>         g_frame_dt {0.0F};
        std::atomic<std::uint64_t> g_prev_ticks {0};
        std::int64_t               g_qpc_frequency = 0;
#pragma clang diagnostic pop

        auto camera_fx() -> std::uintptr_t {
            const auto manager = mem::read<std::uintptr_t>(g_camera_manager);
            return manager == 0 ? 0 : mem::read<std::uintptr_t>(manager + k_camera_fx_slot);
        }

        auto binding_targets(std::uintptr_t binding, std::uintptr_t object) -> bool {
            const auto entries = mem::read<std::uintptr_t>(binding + k_binding_entries);
            const auto count   = mem::read<std::uint16_t>(binding + k_binding_count);
            if (entries == 0) {
                return false;
            }
            for (std::uint16_t i = 0; i < count; ++i) {
                if (mem::read<std::uintptr_t>(entries + (i * k_entry_size) + k_entry_target) ==
                    object) {
                    return true;
                }
            }
            return false;
        }

        // Runs once per rendered frame on the render thread, at the top of the
        // camera interpolation. The sequence tracks run on the game thread, which
        // only reads the smoothed value.
        struct SampleFrameTime {
            [[maybe_unused]] static constexpr std::string_view name = "CameraLean";
            [[maybe_unused]] static void operator()(mem::Registers & /*regs*/) {
                LARGE_INTEGER now {};
                QueryPerformanceCounter(&now);
                const auto ticks = static_cast<std::uint64_t>(now.QuadPart);
                const auto prev  = g_prev_ticks.exchange(ticks, std::memory_order_relaxed);
                if (prev == 0 || g_qpc_frequency == 0 || ticks <= prev) {
                    return;
                }
                const auto dt = static_cast<float>(static_cast<double>(ticks - prev) /
                                                   static_cast<double>(g_qpc_frequency));
                if (dt > k_max_plausible_dt) {
                    return;
                }
                const auto cur = g_frame_dt.load(std::memory_order_relaxed);
                g_frame_dt.store(cur <= 0.0F ? dt : cur + ((dt - cur) * k_dt_alpha),
                                 std::memory_order_relaxed);
            }
        };

        // Right after the dispatcher computed the track's fade weight into xmm0.
        struct ScaleTrackWeight {
            [[maybe_unused]] static constexpr std::string_view name = "CameraLean";
            [[maybe_unused]] static void                       operator()(mem::Registers &regs) {
                const auto descriptor = mem::read<std::uintptr_t>(regs.rbx + k_binding_descriptor);
                if (descriptor == 0 ||
                    (mem::read<std::uint32_t>(descriptor) & k_mode_mask) != k_mode_additive) {
                    return;
                }
                const auto fx = camera_fx();
                if (fx == 0 || !binding_targets(regs.rbx, fx)) {
                    return;
                }
                const auto fps = g_reference_fps.load(std::memory_order_relaxed);
                if (fps <= 0.0F) {
                    regs.xmm0.f32[0] = 0.0F;
                    return;
                }
                const auto dt = g_frame_dt.load(std::memory_order_relaxed);
                if (dt <= 0.0F) {
                    return;
                }
                regs.xmm0.f32[0] *= std::clamp(dt * fps, 0.0F, k_max_scale);
            }
        };
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        const auto fps = std::max(cfg.reference_fps.get(), 0.0F);
        g_reference_fps.store(fps, std::memory_order_relaxed);
        if (fps <= 0.0F) {
            log::get()->trace("CameraLean: lean disabled");
        } else {
            log::get()->trace("CameraLean: reference fps {:.1f}", fps);
        }
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        LARGE_INTEGER freq {};
        QueryPerformanceFrequency(&freq);
        g_qpc_frequency = freq.QuadPart;

        // mov rax, [rip+disp32] inside the function that applies the FX rotation.
        g_camera_manager = mem::x64::read_rel(addrs.camera_manager_load.value() + 3);
        log::get()->trace("CameraLean: camera manager global at 0x{:X}", g_camera_manager);

        auto frame = mem::make_hook<SampleFrameTime>(addrs.camera_interpolate.value());
        if (!frame) {
            log::get()->error("CameraLean: frame hook failed: {}", frame.error());
            return false;
        }
        g_frame_hook = std::move(*frame);

        auto weight = mem::make_hook<ScaleTrackWeight>(addrs.track_weight_site.value());
        if (!weight) {
            log::get()->error("CameraLean: weight hook failed: {}", weight.error());
            g_frame_hook.reset();
            return false;
        }
        g_weight_hook = std::move(*weight);

        on_reload(games::rogue::registry().config<Tag>());
        log::get()->info("CameraLean: installed");
        return true;
    }
} // namespace hooks
