#include "games/rogue/hooks/display_detection.hpp"

#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/rogue/game_data.hpp"
#include "games/rogue/registry.hpp"
#include "games/rogue/structs.hpp"

namespace hooks {
    using games::rogue::MultiMonitor;

    namespace {
        using Data = games::game_data<games::Rogue>;
        using Tag  = games::rogue::DisplayDetectionHook;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        std::atomic<float> g_cached_width {0.0F};
        std::atomic<float> g_cached_height {0.0F};
        mem::MidHook       g_display_hook;
#pragma clang diagnostic pop

        struct DisplayFlagHook {
            [[maybe_unused]] static constexpr std::string_view name = "DisplayDetection";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                const auto *display =
                    reinterpret_cast<const games::rogue::DisplaySettings *>(regs.rbx);
                g_cached_width.store(display->width, std::memory_order_relaxed);
                g_cached_height.store(display->height, std::memory_order_relaxed);

                if (!games::rogue::registry().enabled<Tag>()) {
                    if (regs.xmm1.f32[0] < regs.xmm0.f32[0]) {
                        regs.rdx = regs.rsi;
                    }
                    return;
                }

                auto mode = games::rogue::registry().config<Tag>().multi_monitor.get();
                switch (mode) {
                    case MultiMonitor::ForceSingle:
                        regs.rdx = 0;
                        break;
                    case MultiMonitor::ForceMulti:
                        regs.rdx = 1;
                        break;
                    case MultiMonitor::Auto: {
                        const float w = display->width;
                        const float h = display->height;
                        regs.rdx = (h > 0.0F && (w / h) >= Data::k_triple_screen_threshold) ? 1 : 0;
                        break;
                    }
                    case MultiMonitor::_count:
                    default:
                        std::unreachable();
                }
            }
        };
    } // namespace

    void HookTraits<games::rogue::DisplayDetectionHook>::on_reload(const Config &cfg) {
        const float w = g_cached_width.load(std::memory_order_relaxed);
        const float h = g_cached_height.load(std::memory_order_relaxed);
        if (w == 0.0F && h == 0.0F) {
            return;
        }

        log::get()->info(
            "DisplayDetection: reload mode={}, w={}, h={} — flag written by callback each frame",
            std::to_underlying(cfg.multi_monitor.get()),
            w,
            h);
    }

    auto HookTraits<games::rogue::DisplayDetectionHook>::install(const Addrs &addrs) -> bool {
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
