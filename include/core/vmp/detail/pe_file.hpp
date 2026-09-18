#pragma once

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <iterator>
#include <optional>
#include <span>

namespace vmp::detail {
    template<typename T>
    auto read_at(std::span<const std::uint8_t> buf, std::size_t offset) -> std::optional<T> {
        if (offset + sizeof(T) > buf.size()) {
            return std::nullopt;
        }
        T value {};
        std::copy_n(std::next(buf.begin(), static_cast<std::ptrdiff_t>(offset)),
                    sizeof(T),
                    reinterpret_cast<std::uint8_t *>(&value));
        return value;
    }

    // An RVA equals its file offset only inside the headers, so translating one
    // in the body of the image means walking the section table.
    inline auto rva_to_file_offset(std::span<const std::uint8_t> image, std::uint32_t rva)
        -> std::optional<std::size_t> {
        auto pe_offset = read_at<std::uint32_t>(image, 0x3C);
        if (!pe_offset) {
            return std::nullopt;
        }

        auto section_count  = read_at<std::uint16_t>(image, *pe_offset + 6);
        auto opt_header_len = read_at<std::uint16_t>(image, *pe_offset + 20);
        if (!section_count || !opt_header_len) {
            return std::nullopt;
        }

        constexpr std::size_t coff_header_size = 24;
        constexpr std::size_t section_size     = 40;
        auto                  table            = *pe_offset + coff_header_size + *opt_header_len;

        for (std::size_t i = 0; i < *section_count; ++i) {
            auto entry    = table + (i * section_size);
            auto virt_sz  = read_at<std::uint32_t>(image, entry + 8);
            auto virt_rva = read_at<std::uint32_t>(image, entry + 12);
            auto raw_sz   = read_at<std::uint32_t>(image, entry + 16);
            auto raw_ptr  = read_at<std::uint32_t>(image, entry + 20);
            if (!virt_sz || !virt_rva || !raw_sz || !raw_ptr) {
                return std::nullopt;
            }
            // A section's mapped size can exceed what the file holds, and the
            // reverse happens when raw data is padded out to alignment.
            auto span = std::max(*virt_sz, *raw_sz);
            if (rva >= *virt_rva && rva < *virt_rva + span) {
                return *raw_ptr + (rva - *virt_rva);
            }
        }
        return std::nullopt;
    }
} // namespace vmp::detail
