#pragma once

#include <type_traits>
#include <utility>

#include <Windows.h>

namespace win32 {
    struct NullInvalid {};
    struct NegativeOneInvalid {};

    template<typename Policy = NegativeOneInvalid>
    class UniqueHandle {
        static auto invalid() noexcept -> HANDLE {
            if constexpr (std::is_same_v<Policy, NullInvalid>) {
                return nullptr;
            } else {
                return INVALID_HANDLE_VALUE;
            }
        }

        HANDLE m_handle = invalid();

      public:
        UniqueHandle() noexcept = default;
        explicit UniqueHandle(HANDLE h) noexcept : m_handle(h) {}

        ~UniqueHandle() { reset(); }

        UniqueHandle(const UniqueHandle &)                     = delete;
        auto operator=(const UniqueHandle &) -> UniqueHandle & = delete;

        UniqueHandle(UniqueHandle &&other) noexcept
            : m_handle(std::exchange(other.m_handle, invalid())) {}

        auto operator=(UniqueHandle &&other) noexcept -> UniqueHandle & {
            if (this != &other) {
                reset();
                m_handle = std::exchange(other.m_handle, invalid());
            }
            return *this;
        }

        void reset() noexcept {
            if (m_handle != invalid()) {
                CloseHandle(m_handle);
                m_handle = invalid();
            }
        }

        [[nodiscard]] auto release() noexcept -> HANDLE {
            return std::exchange(m_handle, invalid());
        }

        [[nodiscard]] auto     get() const noexcept -> HANDLE { return m_handle; }
        [[nodiscard]] explicit operator bool() const noexcept { return m_handle != invalid(); }
    };
} // namespace win32
