#include "diagnostics/minidump.hpp"

#include <cstdio>

#include <string_view>

#include <Windows.h>

#include <DbgHelp.h>

#include "logger.hpp" // IWYU pragma: keep

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace diagnostics {
    namespace {
        using MiniDumpWriteDump_t = BOOL(WINAPI *)(HANDLE                          hProcess,
                                                   DWORD                           ProcessId,
                                                   HANDLE                          hFile,
                                                   MINIDUMP_TYPE                   DumpType,
                                                   PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                                   PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                                   PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

        auto get_dump_fn() -> MiniDumpWriteDump_t {
            static auto *h_dbghelp = LoadLibraryA("dbghelp.dll");
            if (h_dbghelp == nullptr) {
                return nullptr;
            }
            static auto *fn = reinterpret_cast<MiniDumpWriteDump_t>(
                GetProcAddress(h_dbghelp, "MiniDumpWriteDump"));
            return fn;
        }

        void build_dump_path(char *out, std::size_t out_size) {
            char    module_path[MAX_PATH] {};
            HMODULE hSelf = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&build_dump_path),
                               &hSelf);
            GetModuleFileNameA(hSelf, module_path, MAX_PATH);

            std::string_view path(module_path);

            auto sep      = path.rfind('\\');
            auto dir      = (sep != std::string_view::npos) ? path.substr(0, sep + 1) : path;
            auto filename = (sep != std::string_view::npos) ? path.substr(sep + 1) : path;
            auto dot      = filename.rfind('.');
            auto stem     = (dot != std::string_view::npos) ? filename.substr(0, dot) : filename;

            SYSTEMTIME st {};
            GetLocalTime(&st);

            std::snprintf(out,
                          out_size,
                          "%.*s%.*s.%04u%02u%02u_%02u%02u%02u.dmp",
                          static_cast<int>(dir.size()),
                          dir.data(),
                          static_cast<int>(stem.size()),
                          stem.data(),
                          st.wYear,
                          st.wMonth,
                          st.wDay,
                          st.wHour,
                          st.wMinute,
                          st.wSecond);
        }
    } // namespace

    void write_minidump(EXCEPTION_POINTERS *ep) {
        auto *fn = get_dump_fn();
        if (fn == nullptr) {
            log::get()->warn("Minidump: dbghelp.dll not available");
            return;
        }

        char dump_path[MAX_PATH] {};
        build_dump_path(dump_path, MAX_PATH);

        HANDLE hFile = CreateFileA(dump_path,
                                   GENERIC_WRITE,
                                   0,
                                   nullptr,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            log::get()->warn("Minidump: failed to create {}", dump_path);
            return;
        }

        MINIDUMP_EXCEPTION_INFORMATION mei {};
        mei.ThreadId          = GetCurrentThreadId();
        mei.ExceptionPointers = ep;
        mei.ClientPointers    = FALSE;

        auto dump_type = static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo |
                                                    MiniDumpWithIndirectlyReferencedMemory);

        BOOL ok =
            fn(GetCurrentProcess(), GetCurrentProcessId(), hFile, dump_type, &mei, nullptr, nullptr);
        CloseHandle(hFile);

        if (ok != FALSE) {
            log::get()->critical("Minidump written: {}", dump_path);
        } else {
            log::get()->warn("Minidump: MiniDumpWriteDump failed (error {})", GetLastError());
        }
    }
} // namespace diagnostics

#pragma clang diagnostic pop
