#include "core/vmp/detail/pe_sections.hpp"

#include <utility>

#include <Windows.h>

#include "core/win32/pe.hpp"

namespace vmp::detail {
    auto find_vmp_sections(HMODULE hModule) -> VmpSections {
        VmpSections result;
        for (auto &sec : win32::enumerate_sections(hModule)) {
            if (sec.name.starts_with(".UBX")) {
                result.vmp.push_back(std::move(sec));
            } else if (sec.name == ".text") {
                result.text = std::move(sec);
            }
        }
        return result;
    }
} // namespace vmp::detail
