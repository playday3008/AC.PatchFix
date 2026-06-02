#include <catch2/catch_test_macros.hpp>

#include <mini/ini.h>

#include "core/hooks/registry/ini_field.hpp"

using namespace hooks;

TEST_CASE("ini_field default value", "[ini_field]") {
    ini_field<float> field("Section", "Key", 3.14F);
    CHECK(field.get() == 3.14F);
}

TEST_CASE("ini_field store and get round-trip", "[ini_field]") {
    ini_field<bool> field("Section", "Enabled", false);
    field.store(true);
    CHECK(field.get() == true);
}

TEST_CASE("ini_field load_from existing key", "[ini_field]") {
    mINI::INIStructure ini;
    ini["Graphics"]["FOV"] = "110.0";

    ini_field<float> field("Graphics", "FOV", 90.0F);
    field.load_from(ini);
    CHECK(field.get() == 110.0F);
}

TEST_CASE("ini_field load_from missing section keeps default", "[ini_field]") {
    mINI::INIStructure ini;

    ini_field<float> field("Missing", "Key", 42.0F);
    field.load_from(ini);
    CHECK(field.get() == 42.0F);
}

TEST_CASE("ini_field load_from missing key keeps default", "[ini_field]") {
    mINI::INIStructure ini;
    ini["Section"]["Other"] = "1";

    ini_field<bool> field("Section", "Missing", false);
    field.load_from(ini);
    CHECK(field.get() == false);
}

TEST_CASE("ini_field load_from bool", "[ini_field]") {
    mINI::INIStructure ini;
    ini["Section"]["Toggle"] = "yes";

    ini_field<bool> field("Section", "Toggle", false);
    field.load_from(ini);
    CHECK(field.get() == true);
}
