#include "mem/protect.hpp"

#include <cstddef>
#include <cstdint>

#include <atomic>
#include <optional>

#include <Windows.h>
#include <winternl.h>

namespace mem {
    namespace {
        using NtProtectVirtualMemory_t = NTSTATUS NTAPI(HANDLE  ProcessHandle,
                                                        PVOID  *BaseAddress,
                                                        PSIZE_T RegionSize,
                                                        ULONG   NewProtection,
                                                        PULONG  OldProtection);

        std::atomic<ProtectMethod> g_method {ProtectMethod::virtual_protect};

        NtProtectVirtualMemory_t *pNtProtectVirtualMemory = nullptr;
        HANDLE                    g_self_process          = nullptr;

        void ensure_nt_protect() {
            if (pNtProtectVirtualMemory != nullptr) {
                return;
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
            pNtProtectVirtualMemory = reinterpret_cast<NtProtectVirtualMemory_t *>(
                GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtProtectVirtualMemory"));
#pragma clang diagnostic pop
            g_self_process = OpenProcess(PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId());
        }

        auto protect_vp(std::uintptr_t addr, std::size_t size, std::uint32_t new_protect)
            -> std::optional<std::uint32_t> {
            DWORD old = 0;
            if (VirtualProtect(reinterpret_cast<void *>(addr), size, new_protect, &old) == 0) {
                return std::nullopt;
            }
            return old;
        }

        auto protect_nt(std::uintptr_t addr, std::size_t size, std::uint32_t new_protect)
            -> std::optional<std::uint32_t> {
            ensure_nt_protect();
            if (pNtProtectVirtualMemory == nullptr || g_self_process == nullptr) {
                return std::nullopt;
            }
            auto      *base   = reinterpret_cast<void *>(addr);
            auto       region = size;
            ULONG      old    = 0;
            const LONG status =
                pNtProtectVirtualMemory(g_self_process, &base, &region, new_protect, &old);
            if (status < 0) {
                return std::nullopt;
            }
            return old;
        }
    } // namespace

    void set_protect_method(ProtectMethod method) {
        g_method.store(method, std::memory_order_release);
        if (method == ProtectMethod::nt_protect) {
            ensure_nt_protect();
        }
    }

    auto protect_method() -> ProtectMethod {
        return g_method.load(std::memory_order_acquire);
    }
} // namespace mem

namespace mem::detail {
    auto protect(std::uintptr_t addr, std::size_t size, std::uint32_t new_protect)
        -> std::optional<std::uint32_t> {
        if (mem::protect_method() == ProtectMethod::nt_protect) {
            return protect_nt(addr, size, new_protect);
        }
        return protect_vp(addr, size, new_protect);
    }

    void unprotect(std::uintptr_t addr, std::size_t size, std::uint32_t old_protect) {
        if (mem::protect_method() == ProtectMethod::nt_protect) {
            protect_nt(addr, size, old_protect);
        } else {
            protect_vp(addr, size, old_protect);
        }
    }
} // namespace mem::detail
