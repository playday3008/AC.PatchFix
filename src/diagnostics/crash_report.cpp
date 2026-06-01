#include "diagnostics/crash_report.hpp"

#include <cstddef>
#include <cstdint>

#include <array>
#include <span>
#include <string_view>

#include <Windows.h>

#include "logger.hpp" // IWYU pragma: keep

#include "diagnostics/minidump.hpp"
#include "diagnostics/stack_walker.hpp"

namespace diagnostics {
    namespace {
        void log_access_violation_detail(const EXCEPTION_RECORD *rec) {
            if (rec->NumberParameters < 2) {
                return;
            }
            auto access_type = rec->ExceptionInformation[0];
            auto fault_addr  = rec->ExceptionInformation[1];

            std::string_view type_str;
            if (access_type == 0) {
                type_str = "READ";
            } else if (access_type == 1) {
                type_str = "WRITE";
            } else if (access_type == 8) {
                type_str = "DEP (execute)";
            } else {
                type_str = "UNKNOWN";
            }

            log::get()->critical("  Access violation: {} at address 0x{:016X}",
                                 type_str,
                                 fault_addr);
        }

        void log_registers(const CONTEXT *ctx) {
            log::get()->critical("  RAX={:016X}  RBX={:016X}  RCX={:016X}  RDX={:016X}",
                                 ctx->Rax,
                                 ctx->Rbx,
                                 ctx->Rcx,
                                 ctx->Rdx);
            log::get()->critical("  RSI={:016X}  RDI={:016X}  RBP={:016X}  RSP={:016X}",
                                 ctx->Rsi,
                                 ctx->Rdi,
                                 ctx->Rbp,
                                 ctx->Rsp);
            log::get()->critical("  R8 ={:016X}  R9 ={:016X}  R10={:016X}  R11={:016X}",
                                 ctx->R8,
                                 ctx->R9,
                                 ctx->R10,
                                 ctx->R11);
            log::get()->critical("  R12={:016X}  R13={:016X}  R14={:016X}  R15={:016X}",
                                 ctx->R12,
                                 ctx->R13,
                                 ctx->R14,
                                 ctx->R15);
            log::get()->critical("  RIP={:016X}  RFLAGS={:016X}", ctx->Rip, ctx->EFlags);
        }

        void log_stack_trace(std::span<const StackFrame> frames) {
            log::get()->critical("  Stack trace ({} frames):", frames.size());
            for (std::size_t i = 0; i < frames.size(); ++i) {
                const auto &frame = frames[i];
                if (frame.module_base != 0) {
                    if (frame.has_symbol) {
                        log::get()->critical("    #{:02d}  {}+0x{:X} ({} +0x{:X})",
                                             i,
                                             frame.module_name.data(),
                                             frame.module_offset,
                                             frame.symbol_name.data(),
                                             frame.symbol_offset);
                    } else {
                        log::get()->critical("    #{:02d}  {}+0x{:X}",
                                             i,
                                             frame.module_name.data(),
                                             frame.module_offset);
                    }
                } else {
                    log::get()->critical("    #{:02d}  0x{:016X}", i, frame.address);
                }
            }
        }

        auto fault_filter_impl(EXCEPTION_POINTERS *ep, std::string_view context) -> int {
            auto code = static_cast<std::uint32_t>(ep->ExceptionRecord->ExceptionCode);
            if (!is_hardware_exception(code) || code == EXCEPTION_STACK_OVERFLOW) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            log_crash_report(ep, context);
            write_minidump(ep);
            return EXCEPTION_EXECUTE_HANDLER;
        }
    } // namespace

    auto exception_code_name(std::uint32_t code) -> std::string_view {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:
                return "ACCESS_VIOLATION";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                return "ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_DATATYPE_MISALIGNMENT:
                return "DATATYPE_MISALIGNMENT";
            case EXCEPTION_FLT_DENORMAL_OPERAND:
                return "FLT_DENORMAL_OPERAND";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                return "FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_INEXACT_RESULT:
                return "FLT_INEXACT_RESULT";
            case EXCEPTION_FLT_INVALID_OPERATION:
                return "FLT_INVALID_OPERATION";
            case EXCEPTION_FLT_OVERFLOW:
                return "FLT_OVERFLOW";
            case EXCEPTION_FLT_STACK_CHECK:
                return "FLT_STACK_CHECK";
            case EXCEPTION_FLT_UNDERFLOW:
                return "FLT_UNDERFLOW";
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                return "ILLEGAL_INSTRUCTION";
            case EXCEPTION_IN_PAGE_ERROR:
                return "IN_PAGE_ERROR";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                return "INT_DIVIDE_BY_ZERO";
            case EXCEPTION_INT_OVERFLOW:
                return "INT_OVERFLOW";
            case EXCEPTION_PRIV_INSTRUCTION:
                return "PRIV_INSTRUCTION";
            case EXCEPTION_STACK_OVERFLOW:
                return "STACK_OVERFLOW";
            default:
                return "UNKNOWN";
        }
    }

    auto is_hardware_exception(std::uint32_t code) -> bool {
        return exception_code_name(code) != "UNKNOWN";
    }

    void log_crash_report(EXCEPTION_POINTERS *ep, std::string_view context_name) {
        auto *rec    = ep->ExceptionRecord;
        auto  code   = static_cast<std::uint32_t>(rec->ExceptionCode);
        auto  logger = log::get();

        logger->critical("=== CRASH in {} ===", context_name);
        logger->critical("  Exception: {} (0x{:08X}) at 0x{:016X}",
                         exception_code_name(code),
                         code,
                         reinterpret_cast<std::uintptr_t>(rec->ExceptionAddress));

        if (code == EXCEPTION_ACCESS_VIOLATION) {
            log_access_violation_detail(rec);
        }

        log_registers(ep->ContextRecord);

        std::array<StackFrame, k_max_frames> buf {};
        auto                                 frames = capture_stack(ep->ContextRecord, buf);
        resolve_modules(frames);
        resolve_symbols(frames);
        log_stack_trace(frames);

        logger->critical("=== END CRASH REPORT ===");
        logger->flush();
    }

    void log_crash_report_lightweight(EXCEPTION_POINTERS *ep) {
        auto *rec    = ep->ExceptionRecord;
        auto  code   = static_cast<std::uint32_t>(rec->ExceptionCode);
        auto *ctx    = ep->ContextRecord;
        auto  logger = log::get();

        auto fault_addr = reinterpret_cast<std::uintptr_t>(rec->ExceptionAddress);

        HMODULE                    hModule = nullptr;
        std::array<char, MAX_PATH> module_path {};
        bool has_module = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                                 GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                             reinterpret_cast<LPCSTR>(fault_addr),
                                             &hModule) != 0;
        if (has_module) {
            auto base          = reinterpret_cast<std::uintptr_t>(hModule);
            auto module_offset = fault_addr - base;
            if (GetModuleFileNameA(hModule,
                                   module_path.data(),
                                   static_cast<DWORD>(module_path.size())) == 0) {
                has_module = false;
            } else {
                const std::string_view path(module_path.data());
                const auto             sep = path.rfind('\\');
                const auto module_name =
                    (sep != std::string_view::npos) ? path.substr(sep + 1) : path;
                logger->critical("VEH: {} (0x{:08X}) at {}+0x{:X}",
                                 exception_code_name(code),
                                 code,
                                 module_name,
                                 module_offset);
            }
        }
        if (!has_module) {
            logger->critical("VEH: {} (0x{:08X}) at 0x{:016X}",
                             exception_code_name(code),
                             code,
                             fault_addr);
        }

        logger->critical("VEH: RIP={:016X} RSP={:016X} RBP={:016X}", ctx->Rip, ctx->Rsp, ctx->Rbp);
        logger->flush();
    }

    auto callback_fault_filter(EXCEPTION_POINTERS *ep) -> int {
        return fault_filter_impl(ep, "hook callback");
    }

    auto install_fault_filter(EXCEPTION_POINTERS *ep, std::string_view hook_name) -> int {
        return fault_filter_impl(ep, hook_name);
    }
} // namespace diagnostics
