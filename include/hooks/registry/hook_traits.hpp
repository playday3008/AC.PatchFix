#pragma once

namespace hooks {
    template<typename Tag>
    struct HookTraits;

    template<typename Tag>
    concept HasOnReload =
        requires(const typename HookTraits<Tag>::Config &cfg) { HookTraits<Tag>::on_reload(cfg); };
} // namespace hooks
