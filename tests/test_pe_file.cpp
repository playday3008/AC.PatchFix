#include <cstdint>

#include <array>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/vmp/detail/pe_file.hpp"

namespace {
    // Minimal PE far enough along to exercise the section walk: e_lfanew, a COFF
    // header giving the section count and optional-header size, and one section.
    auto make_image() -> std::vector<std::uint8_t> {
        std::vector<std::uint8_t> img(0x400, 0);

        constexpr std::uint32_t pe = 0x80;
        img[0x3C]                  = pe;

        auto put16 = [&img](std::size_t off, std::uint16_t v) -> void {
            img[off]     = static_cast<std::uint8_t>(v & 0xFF);
            img[off + 1] = static_cast<std::uint8_t>(v >> 8);
        };
        auto put32 = [&img](std::size_t off, std::uint32_t v) -> void {
            for (std::size_t i = 0; i < 4; ++i) {
                img[off + i] = static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF);
            }
        };

        put16(pe + 6, 2);  // section count
        put16(pe + 20, 8); // optional header size

        auto table = pe + 24 + 8;
        // .text: rva 0x1000 size 0x2000, raw at 0x400 size 0x2000
        put32(table + 8, 0x2000);
        put32(table + 12, 0x1000);
        put32(table + 16, 0x2000);
        put32(table + 20, 0x400);
        // .data: rva 0x4000 virtual 0x1000 but only 0x200 on disk, raw at 0x2400
        put32(table + 40 + 8, 0x1000);
        put32(table + 40 + 12, 0x4000);
        put32(table + 40 + 16, 0x200);
        put32(table + 40 + 20, 0x2400);
        return img;
    }
} // namespace

TEST_CASE("rva_to_file_offset translates through the section table") {
    auto img = make_image();

    REQUIRE(vmp::detail::rva_to_file_offset(img, 0x1000) == 0x400);
    REQUIRE(vmp::detail::rva_to_file_offset(img, 0x1234) == 0x634);
    REQUIRE(vmp::detail::rva_to_file_offset(img, 0x4000) == 0x2400);
}

TEST_CASE("rva_to_file_offset spans a section larger in memory than on disk") {
    auto img = make_image();
    // Past the 0x200 bytes of raw data but inside the 0x1000 virtual size: the
    // section still owns this RVA, so the walk must not skip past it.
    REQUIRE(vmp::detail::rva_to_file_offset(img, 0x4800) == 0x2C00);
}

TEST_CASE("rva_to_file_offset rejects an RVA in no section") {
    auto img = make_image();
    REQUIRE_FALSE(vmp::detail::rva_to_file_offset(img, 0x500).has_value());
    REQUIRE_FALSE(vmp::detail::rva_to_file_offset(img, 0x9000).has_value());
}

TEST_CASE("rva_to_file_offset refuses a truncated image") {
    const std::vector<std::uint8_t> tiny(0x20, 0);
    REQUIRE_FALSE(vmp::detail::rva_to_file_offset(tiny, 0x1000).has_value());
}

TEST_CASE("read_at refuses to run off the end") {
    const std::array<std::uint8_t, 4> buf {1, 2, 3, 4};
    REQUIRE(vmp::detail::read_at<std::uint32_t>(buf, 0).has_value());
    REQUIRE_FALSE(vmp::detail::read_at<std::uint32_t>(buf, 1).has_value());
}
