#include "diagnostics/stack_walker.hpp"

#include <cstring>

#include <algorithm>
#include <string_view>

#include <Windows.h>

#include <DbgHelp.h>

namespace diagnostics {
    auto capture_stack(const CONTEXT *ctx, std::span<StackFrame> buf) -> std::span<StackFrame> {
        CONTEXT     local_ctx {};
        std::size_t count = 0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
        std::memcpy(&local_ctx, ctx, sizeof(CONTEXT));
#pragma clang diagnostic pop

        while (count < buf.size()) {
            if (local_ctx.Rip == 0) {
                break;
            }

            buf[count].address = local_ctx.Rip;
            count++;

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
        return buf.first(count);
    }

    void resolve_modules(std::span<StackFrame> frames) {
        for (auto &frame : frames) {
            HMODULE hModule = nullptr;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(frame.address),
                                   &hModule) == FALSE) {
                continue;
            }
            frame.module_base   = reinterpret_cast<std::uintptr_t>(hModule);
            frame.module_offset = frame.address - frame.module_base;

            if (GetModuleFileNameA(hModule, frame.module_name.data(), frame.module_name.size()) ==
                FALSE) {
                continue;
            }

            const std::string_view path(frame.module_name.data());
            const auto             sep = path.rfind('\\');
            if (sep != std::string_view::npos) {
                auto                       filename = path.substr(sep + 1);
                std::array<char, MAX_PATH> tmp {};
                std::copy_n(filename.data(), filename.size() + 1, tmp.data());
                frame.module_name = tmp;
            }
        }
    }

    void resolve_symbols(std::span<StackFrame> frames) {
        static auto *h_dbghelp = LoadLibraryA("dbghelp.dll");
        if (h_dbghelp == nullptr) {
            return;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
        static auto *pSymInitialize =
            reinterpret_cast<decltype(SymInitialize) *>(GetProcAddress(h_dbghelp, "SymInitialize"));
        static auto *pSymFromAddr =
            reinterpret_cast<decltype(SymFromAddr) *>(GetProcAddress(h_dbghelp, "SymFromAddr"));
#pragma clang diagnostic pop

        if (pSymInitialize == nullptr || pSymFromAddr == nullptr) {
            return;
        }

        static const bool initialized =
            (pSymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE);
        if (!initialized) {
            return;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        alignas(SYMBOL_INFO) std::array<std::byte, sizeof(SYMBOL_INFO) + k_max_sym_len> buf {};
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(buf.data());
#pragma clang diagnostic pop
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen   = k_max_sym_len - 1;

        for (auto &frame : frames) {
            DWORD64 displacement = 0;
            if (pSymFromAddr(GetCurrentProcess(), frame.address, &displacement, symbol) != FALSE) {
                auto name_len = std::char_traits<char>::length(symbol->Name);
                auto copy_len = std::min(name_len, k_max_sym_len - 1);
                std::copy_n(symbol->Name, copy_len, frame.symbol_name.data());
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                frame.symbol_name[copy_len] = '\0';
                frame.symbol_offset         = displacement;
                frame.has_symbol            = true;
            }
        }
    }
} // namespace diagnostics
