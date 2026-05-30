#pragma once

#include <cstdint>

namespace mem {
    namespace detail {
        template<typename Sig>
        struct invoke_impl;

        template<typename Ret, typename... Args>
        struct invoke_impl<Ret(Args...)> {
            static auto call(uintptr_t addr, Args... args) -> Ret {
                return reinterpret_cast<Ret (*)(Args...)>(addr)(args...);
            }
        };
    } // namespace detail

    template<typename Sig, typename... Args>
    auto invoke(uintptr_t addr, Args... args) {
        return detail::invoke_impl<Sig>::call(addr, args...);
    }
} // namespace mem
