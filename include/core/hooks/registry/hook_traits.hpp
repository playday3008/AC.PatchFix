#pragma once

namespace hooks {
    template<typename Tag>
    struct HookTraits {
        static_assert(sizeof(Tag) == 0, "HookTraits<Tag> must be specialized for each hook");
    };

    template<typename Tag>
    concept HasOnReload =
        requires(const HookTraits<Tag>::Config &cfg) { HookTraits<Tag>::on_reload(cfg); };
} // namespace hooks
