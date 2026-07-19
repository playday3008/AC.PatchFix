#include "games/unity/hooks/freeze_fov.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/unity/registry.hpp"

namespace hooks {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_freeze_fov_hook;
#pragma clang diagnostic pop

        using Tag = games::unity::FreezeFOVHook;

        constexpr float k_fov_min = 0.2F;
        constexpr float k_fov_max = 2.0F;

        struct FreezeFOVFunctor {
            [[maybe_unused]] static constexpr std::string_view name = "FreezeFOV";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::unity::registry().enabled<Tag>()) {
                    return; // original `movss [r14+70h], xmm1` runs -> vanilla FOV
                }
                const float fov  = std::clamp(games::unity::registry().config<Tag>().fov.get(),
                                              k_fov_min,
                                              k_fov_max);
                regs.xmm1.f32[0] = fov; // re-executed movss then stores our value to [r14+0x70]
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.freeze_fov_site.value();
        log::get()->trace("Unity FreezeFOVHook: installing at 0x{:X}", addr);

        if (auto h = mem::make_hook<FreezeFOVFunctor>(addr)) {
            g_freeze_fov_hook = std::move(*h);
        } else {
            log::get()->error("Unity FreezeFOVHook: hook failed: {}", h.error());
            return false;
        }

        log::get()->info("Unity FreezeFOVHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
