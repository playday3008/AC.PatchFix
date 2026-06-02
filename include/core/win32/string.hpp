#pragma once

#include <cstddef>

#include <string>

#include <Windows.h>

namespace win32 {
    inline auto wchar_to_utf8(LPCWCH src, int src_len) -> std::string {
        const int len = WideCharToMultiByte(CP_UTF8, 0, src, src_len, nullptr, 0, nullptr, nullptr);
        if (len <= 0) {
            return {};
        }
        std::string result(static_cast<std::size_t>(len), '\0');
        WideCharToMultiByte(CP_UTF8, 0, src, src_len, result.data(), len, nullptr, nullptr);
        return result;
    }
} // namespace win32
