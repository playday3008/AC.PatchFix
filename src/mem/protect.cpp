#include "mem/protect.hpp"

#include <cstdint>

#include <atomic>

#include <Windows.h>

namespace mem {

    namespace {

        using NtProtectFn = NTSTATUS(NTAPI *)(HANDLE, PVOID *, PSIZE_T, ULONG, PULONG);

        std::atomic<ProtectMethod> g_method {ProtectMethod::virtual_protect};

        NtProtectFn g_nt_protect   = nullptr;
        HANDLE      g_self_process = nullptr;

        void ensure_nt_protect() {
            if (g_nt_protect != nullptr) {
                return;
            }
            g_nt_protect = reinterpret_cast<NtProtectFn>(
                GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtProtectVirtualMemory"));
            g_self_process = OpenProcess(PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId());
        }

        auto protect_vp(uintptr_t addr, size_t size, uint32_t new_protect)
            -> std::optional<uint32_t> {
            DWORD old = 0;
            if (VirtualProtect(reinterpret_cast<void *>(addr), size, new_protect, &old) == 0) {
                return std::nullopt;
            }
            return old;
        }

        auto protect_nt(uintptr_t addr, size_t size, uint32_t new_protect)
            -> std::optional<uint32_t> {
            ensure_nt_protect();
            if (g_nt_protect == nullptr || g_self_process == nullptr) {
                return std::nullopt;
            }
            auto    *base   = reinterpret_cast<void *>(addr);
            auto     region = size;
            ULONG    old    = 0;
            NTSTATUS status = g_nt_protect(g_self_process, &base, &region, new_protect, &old);
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

    auto protect(uintptr_t addr, size_t size, uint32_t new_protect) -> std::optional<uint32_t> {
        if (mem::protect_method() == ProtectMethod::nt_protect) {
            return protect_nt(addr, size, new_protect);
        }
        return protect_vp(addr, size, new_protect);
    }

    void unprotect(uintptr_t addr, size_t size, uint32_t old_protect) {
        if (mem::protect_method() == ProtectMethod::nt_protect) {
            protect_nt(addr, size, old_protect);
        } else {
            protect_vp(addr, size, old_protect);
        }
    }

} // namespace mem::detail
