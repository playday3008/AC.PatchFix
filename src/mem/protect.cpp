#include "mem/protect.hpp"

#include <cstddef>
#include <cstdint>

#include <atomic>
#include <optional>

#include <Windows.h>
#include <winternl.h>

#include "win32/unique_handle.hpp"

namespace mem {
    namespace {
        using NtProtectVirtualMemory_t = NTSTATUS NTAPI(HANDLE  ProcessHandle,
                                                        PVOID  *BaseAddress,
                                                        PSIZE_T RegionSize,
                                                        ULONG   NewProtection,
                                                        PULONG  OldProtection);

        std::atomic<ProtectMethod> g_method {ProtectMethod::virtual_protect};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        NtProtectVirtualMemory_t               *pNtProtectVirtualMemory = nullptr;
        win32::UniqueHandle<win32::NullInvalid> g_self_process;
#pragma clang diagnostic pop

        void ensure_nt_protect() {
            if (pNtProtectVirtualMemory != nullptr) {
                return;
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
            pNtProtectVirtualMemory = reinterpret_cast<NtProtectVirtualMemory_t *>(
                GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtProtectVirtualMemory"));
#pragma clang diagnostic pop
            g_self_process = win32::UniqueHandle<win32::NullInvalid>(
                OpenProcess(PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId()));
        }

        auto protect_vp(std::uintptr_t addr, std::size_t size, std::uint32_t new_protect)
            -> std::optional<std::uint32_t> {
            DWORD old = 0;
            if (VirtualProtect(reinterpret_cast<void *>(addr), size, new_protect, &old) == FALSE) {
                return std::nullopt;
            }
            return old;
        }

        auto protect_nt(std::uintptr_t addr, std::size_t size, std::uint32_t new_protect)
            -> std::optional<std::uint32_t> {
            ensure_nt_protect();
            if (pNtProtectVirtualMemory == nullptr || !g_self_process) {
                return std::nullopt;
            }
            auto          *base   = reinterpret_cast<void *>(addr);
            auto           region = size;
            ULONG          old    = 0;
            const NTSTATUS status =
                pNtProtectVirtualMemory(g_self_process.get(), &base, &region, new_protect, &old);
            if (!NT_SUCCESS(status)) {
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
