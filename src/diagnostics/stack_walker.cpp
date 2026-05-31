#include "diagnostics/stack_walker.hpp"

#include <cstring>

#include <Windows.h>

#include <DbgHelp.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace diagnostics {
    auto capture_stack(const CONTEXT *ctx) -> StackTrace {
        StackTrace trace {};
        CONTEXT    local_ctx {};
        std::memcpy(&local_ctx, ctx, sizeof(CONTEXT));

        while (trace.count < k_max_frames) {
            if (local_ctx.Rip == 0) {
                break;
            }

            trace.frames[trace.count].address = local_ctx.Rip;
            trace.count++;

            DWORD64 image_base = 0;
            auto   *fn_entry   = RtlLookupFunctionEntry(local_ctx.Rip, &image_base, nullptr);
            if (fn_entry == nullptr) {
                break;
            }

            void   *handler_data      = nullptr;
            ULONG64 establisher_frame = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                             image_base,
                             local_ctx.Rip,
                             fn_entry,
                             &local_ctx,
                             &handler_data,
                             &establisher_frame,
                             nullptr);
        }
        return trace;
    }

    void resolve_modules(StackTrace &trace) {
        for (std::size_t i = 0; i < trace.count; ++i) {
            auto   &frame   = trace.frames[i];
            HMODULE hModule = nullptr;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(frame.address),
                                   &hModule) == 0) {
                continue;
            }
            frame.module_base   = reinterpret_cast<std::uintptr_t>(hModule);
            frame.module_offset = frame.address - frame.module_base;
            GetModuleFileNameA(hModule, frame.module_name, k_max_name_len);

            auto *last_sep = std::strrchr(frame.module_name, '\\');
            if (last_sep != nullptr) {
                std::memmove(frame.module_name, last_sep + 1, std::strlen(last_sep + 1) + 1);
            }
        }
    }

    void resolve_symbols(StackTrace &trace) {
        using SymInitialize_t = BOOL(WINAPI *)(HANDLE, PCSTR, BOOL);
        using SymFromAddr_t   = BOOL(WINAPI *)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);

        static auto *h_dbghelp = LoadLibraryA("dbghelp.dll");
        if (h_dbghelp == nullptr) {
            return;
        }

        static auto *p_sym_init =
            reinterpret_cast<SymInitialize_t>(GetProcAddress(h_dbghelp, "SymInitialize"));
        static auto *p_sym_from_addr =
            reinterpret_cast<SymFromAddr_t>(GetProcAddress(h_dbghelp, "SymFromAddr"));

        if (p_sym_init == nullptr || p_sym_from_addr == nullptr) {
            return;
        }

        static bool initialized = (p_sym_init(GetCurrentProcess(), nullptr, TRUE) != FALSE);
        if (!initialized) {
            return;
        }

        alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + k_max_sym_len] {};
        auto                     *symbol = reinterpret_cast<SYMBOL_INFO *>(buf);
        symbol->SizeOfStruct             = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen               = k_max_sym_len - 1;

        for (std::size_t i = 0; i < trace.count; ++i) {
            auto   &frame        = trace.frames[i];
            DWORD64 displacement = 0;
            if (p_sym_from_addr(GetCurrentProcess(), frame.address, &displacement, symbol) !=
                FALSE) {
                std::strncpy(frame.symbol_name, symbol->Name, k_max_sym_len - 1);
                frame.symbol_name[k_max_sym_len - 1] = '\0';
                frame.symbol_offset                  = displacement;
                frame.has_symbol                     = true;
            }
        }
    }
} // namespace diagnostics

#pragma clang diagnostic pop
