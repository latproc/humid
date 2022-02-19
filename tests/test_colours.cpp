#include "gtest/gtest.h"
#include <nanogui/common.h>
#include <utility>
#include <value.h>

using ColourResult = std::pair<nanogui::Color, std::string>;
ColourResult colourFromString(const std::string &colour);

bool isNull(const Value &value) { return value.kind == Value::t_empty; }

namespace {

TEST(Colours, CanMakeFromString) {
    nanogui::Color white{1.0f, 1.0f, 1.0f, 1.0f};
    auto from_short_rgb_string = colourFromString("#fff");
    auto from_short_rgba_string = colourFromString("#ffFF");
    auto from_rgb_string = colourFromString("#ffFfff");
    auto from_rgba_string = colourFromString("#ffFFffff");
    EXPECT_TRUE(from_short_rgb_string.second.empty());
    EXPECT_EQ(from_short_rgb_string.first, white);
    EXPECT_EQ(from_short_rgba_string.first, white);
    EXPECT_EQ(from_rgb_string.first, white);
    EXPECT_EQ(from_rgba_string.first, white);

    nanogui::Color dark_grey{0.2f, 0.2f, 0.2f, 0.5f};
    auto grey_from_rgba = colourFromString("#33333380").first;
    EXPECT_NEAR(dark_grey.r(), grey_from_rgba.r(), 0.005);
    EXPECT_NEAR(dark_grey.g(), grey_from_rgba.g(), 0.005);
    EXPECT_NEAR(dark_grey.b(), grey_from_rgba.b(), 0.005);
    EXPECT_NEAR(dark_grey.w(), grey_from_rgba.w(), 0.005);

    nanogui::Color grey{0.4f, 0.4f, 0.4f, 1.0f};
    auto short_intensity = colourFromString("&6f").first;
    EXPECT_NEAR(grey.r(), short_intensity.r(), 0.005);
    EXPECT_NEAR(grey.g(), short_intensity.g(), 0.005);
    EXPECT_NEAR(grey.b(), short_intensity.b(), 0.005);
    EXPECT_NEAR(grey.w(), short_intensity.w(), 0.005);

    nanogui::Color grey2{0.4f, 0.4f, 0.4f, 0.5f};
    auto intensity = colourFromString("&6680").first;
    EXPECT_NEAR(grey2.r(), intensity.r(), 0.005);
    EXPECT_NEAR(grey2.g(), intensity.g(), 0.005);
    EXPECT_NEAR(grey2.b(), intensity.b(), 0.005);
    EXPECT_NEAR(grey2.w(), intensity.w(), 0.005);
}

} // namespace
