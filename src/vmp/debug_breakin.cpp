#include "core/vmp/debug_breakin.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <vector>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"
#include "core/vmp/detail/pe_file.hpp"
#include "core/win32/pe.hpp"

namespace vmp {
    namespace {
        // The redirect observed in this game is a 5-byte jmp rel32 into an
        // anonymous trampoline page. Comparing further than the one patched
        // instruction risks calling a legitimate difference a patch.
        constexpr std::size_t k_prologue_bytes = 5;

        auto read_file(const std::filesystem::path &path) -> std::vector<std::uint8_t> {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                return {};
            }
            return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        }

    } // namespace

    auto breakin_state_name(BreakinState state) -> const char * {
        switch (state) {
            case BreakinState::clean:
                return "clean";
            case BreakinState::restored:
                return "restored";
            case BreakinState::patched:
                return "patched";
            case BreakinState::unavailable:
            default:
                return "unavailable";
        }
    }

    auto restore_debug_breakin() -> BreakinState {
        auto *ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll == nullptr) {
            return BreakinState::unavailable;
        }

        // The mapped export table still resolves even with the function patched:
        // a hook rewrites the code, not the directory that points at it.
        auto *live = GetProcAddress(ntdll, "DbgUiRemoteBreakin");
        if (live == nullptr) {
            return BreakinState::unavailable;
        }
        auto addr = reinterpret_cast<std::uintptr_t>(live);
        auto rva  = static_cast<std::uint32_t>(addr - reinterpret_cast<std::uintptr_t>(ntdll));

        auto image = read_file(win32::get_module_path(ntdll));
        if (image.empty()) {
            return BreakinState::unavailable;
        }

        auto offset = detail::rva_to_file_offset(image, rva);
        if (!offset || *offset + k_prologue_bytes > image.size()) {
            return BreakinState::unavailable;
        }

        std::array<std::uint8_t, k_prologue_bytes> original {};
        std::copy_n(image.begin() + static_cast<std::ptrdiff_t>(*offset),
                    k_prologue_bytes,
                    original.begin());

        const auto *current = reinterpret_cast<const std::uint8_t *>(addr);
        if (std::equal(original.begin(), original.end(), current)) {
            log::get()->trace("[VMP] DbgUiRemoteBreakin is unpatched");
            return BreakinState::clean;
        }

        log::get()->info("[VMP] DbgUiRemoteBreakin patched at 0x{:X}, restoring", addr);
        if (!mem::write(addr, original.data(), original.size())) {
            log::get()->warn("[VMP] Could not restore DbgUiRemoteBreakin");
            return BreakinState::patched;
        }

        return BreakinState::restored;
    }
} // namespace vmp
