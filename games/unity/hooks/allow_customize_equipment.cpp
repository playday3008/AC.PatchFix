#include "games/unity/hooks/allow_customize_equipment.hpp"

#include <cstdint>

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
        mem::MidHook g_customize_hook;
#pragma clang diagnostic pop

        using Tag = games::unity::AllowCustomizeEquipmentHook;

        struct NavStateNormalizeFunctor {
            [[maybe_unused]] static constexpr std::string_view name = "AllowCustomizeEquipment";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::unity::registry().enabled<Tag>()) {
                    return;
                }
                // Original: test edx,edx; jnz; mov [r8],1 (flag set only when edx==0).
                // Treat "on ledge/peak, Low Profile" (edx==3) the same as edx==0 so the
                // unchanged original logic sets the "state valid" flag on ledges/peaks.
                // edx is dead after the test within this tiny function, so clobbering is safe.
                if (static_cast<std::uint32_t>(regs.rdx) == 3) {
                    regs.rdx = 0;
                }
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr = addrs.customize_equip_navcheck.value();
        log::get()->trace("Unity AllowCustomizeEquipmentHook: installing at 0x{:X}", addr);

        if (auto h = mem::make_hook<NavStateNormalizeFunctor>(addr)) {
            g_customize_hook = std::move(*h);
        } else {
            log::get()->error("Unity AllowCustomizeEquipmentHook: hook failed: {}", h.error());
            return false;
        }

        log::get()->info("Unity AllowCustomizeEquipmentHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
