#pragma once

#include <cstdint>

#include <utility>

namespace mem {
    namespace detail {
        template<typename Sig>
        struct invoke_impl;

        template<typename Ret, typename... Args>
        struct invoke_impl<Ret(Args...)> {
            static auto call(std::uintptr_t addr, Args... args) -> Ret {
                return reinterpret_cast<Ret (*)(Args...)>(addr)(args...);
            }
        };
    } // namespace detail

    template<typename Sig, typename... Args>
    auto invoke(std::uintptr_t addr, Args &&...args) -> decltype(auto) {
        return detail::invoke_impl<Sig>::call(addr, std::forward<Args>(args)...);
    }
} // namespace mem
