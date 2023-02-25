/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include <cstdint>
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include "../3rdparty/stb/stb_truetype.h"

struct Point;
struct Colour;
struct PixelBuffer;

namespace font {

struct Glyph
{
    int height{}, width{}, y0{}, advance{};
    std::span<std::uint8_t> bitmap;
};

class Font
{
    std::array<Glyph, 256> glyph;
    float scale;
    int baseline;
    stbtt_fontinfo font_info;
    std::vector<std::byte> font_data;

public:
    Font(std::vector<std::byte> data, const int font_size);

    const auto GetScale() const { return scale; }
    const auto GetBaseLine() const { return baseline; }
    const Glyph& GetGlyph(const int ch);
};

void DrawText(PixelBuffer& pb, Font& font, const Point& p, const Colour& colour, std::string_view text);
int GetTextWidth(Font& font, std::string_view text);

}
