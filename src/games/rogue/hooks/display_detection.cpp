#include "hooks/common/display_detection.hpp"

#include <cstdint>

#include <atomic>
#include <utility>

#include "logger.hpp" // IWYU pragma: keep
#include "mem/hook.hpp"

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"

namespace hooks {
    namespace {
        using G    = games::Rogue;
        using Data = games::game_data<G>;
        using Tag  = DisplayDetectionHook<G>;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        std::atomic<uintptr_t> g_display_object {0};
        mem::MidHook g_display_hook;
#pragma clang diagnostic pop

        struct DisplayFlagHook {
            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                g_display_object.store(regs.rbx, std::memory_order_relaxed);

                if (!games::rogue::g_registry.enabled<Tag>()) {
                    if (regs.xmm1.f32[0] < regs.xmm0.f32[0]) {
                        regs.rdx = regs.rsi;
                    }
                    return;
                }

                auto mode = games::rogue::g_registry.config<Tag>().multi_monitor.get();
                switch (mode) {
                    case MultiMonitor::ForceSingle:
                        regs.rdx = 0;
                        break;
                    case MultiMonitor::ForceMulti:
                        regs.rdx = 1;
                        break;
                    case MultiMonitor::Auto: {
                        float w  = *reinterpret_cast<float *>(regs.rbx + 0x10);
                        float h  = *reinterpret_cast<float *>(regs.rbx + 0x14);
                        regs.rdx = (h > 0.0F && (w / h) >= Data::k_triple_screen_threshold) ? 1 : 0;
                        break;
                    }
                    default:
                        std::unreachable();
                }
            }
        };
    } // namespace

    template<>
    void HookTraits<DisplayDetectionHook<games::Rogue>>::on_reload(const Config &cfg) {
        uintptr_t obj = g_display_object.load(std::memory_order_relaxed);
        if (obj == 0) {
            return;
        }

        auto    mode = cfg.multi_monitor.get();
        uint8_t flag = 0;
        switch (mode) {
            case MultiMonitor::ForceSingle:
                flag = 0;
                break;
            case MultiMonitor::ForceMulti:
                flag = 1;
                break;
            case MultiMonitor::Auto: {
                float w = *reinterpret_cast<float *>(obj + 0x10);
                float h = *reinterpret_cast<float *>(obj + 0x14);
                flag    = (h > 0.0F && (w / h) >= Data::k_triple_screen_threshold) ? 1 : 0;
                break;
            }
            default:
                std::unreachable();
        }

        *reinterpret_cast<uint8_t *>(obj + 0x18) = flag;
        if (flag != 0) {
            float w = *reinterpret_cast<float *>(obj + 0x10);
            *reinterpret_cast<uint32_t *>(obj + 0x1C) =
                static_cast<uint32_t>(w * Data::k_multi_monitor_split);
        }

        log::get()->info("DisplayDetection: poked flag={}, singleWidth={}",
                         flag,
                         (flag != 0) ? *reinterpret_cast<uint32_t *>(obj + 0x1C) : 0);
    }

    template<>
    auto HookTraits<DisplayDetectionHook<games::Rogue>>::install(const Addrs &addrs) -> bool {
        log::get()->trace("DisplayDetectionHook: installing at 0x{:X}", addrs.display_flag.value());
        auto addr = addrs.display_flag.value();
        if (auto h = mem::make_hook<DisplayFlagHook>(addr, addr + 7)) {
            g_display_hook = std::move(*h);
        } else {
            log::get()->error("DisplayDetectionHook: hook failed: {}", h.error());
            return false;
        }
        log::get()->trace("DisplayDetectionHook: installed");
        return true;
    }
} // namespace hooks
