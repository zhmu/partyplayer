/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "font.h"
#include <stdexcept>
#include "types.h"
#include "pixelbuffer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../3rdparty/stb/stb_truetype.h"

namespace font {

    Font::Font(std::vector<std::byte> data, const int font_size)
        : font_data(std::move(data))
    {
        if (!stbtt_InitFont(&font_info, reinterpret_cast<const unsigned char*>(font_data.data()), 0))
            throw std::runtime_error("unable to initialize font");

        scale = stbtt_ScaleForPixelHeight(&font_info, font_size);
        int ascent;
        stbtt_GetFontVMetrics(&font_info, &ascent, 0, 0);
        baseline = static_cast<int>(ascent * scale);
    }

    const Glyph& Font::GetGlyph(const int ch)
    {
        auto& g = glyph[ch];
        if (g.bitmap.empty()) {
            int h, w;
            const auto bitmap = stbtt_GetCodepointBitmap(&font_info, 0, scale, ch, &w, &h, 0, 0);
            float x_shift = 0.0f;
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBoxSubpixel( &font_info, ch, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font_info, ch, &advance, &lsb);

            g = Glyph{h, w, y0, advance, {&bitmap[0], &bitmap[h * w]}};
        }
        return g;
    }

    void DrawText(PixelBuffer& pb, Font& font, const Point& p, const Colour& colour, std::string_view text)
    {
        Point current{p};
        const auto baseline = font.GetBaseLine();
        for (const auto ch : text) {
            const auto& glyph = font.GetGlyph(ch);

            const Point renderPoint{current + Point{0, baseline + glyph.y0}};
            for (int y = 0; y < glyph.height; ++y) {
                for (int x = 0; x < glyph.width; ++x) {
                    const auto v = glyph.bitmap[y * glyph.width + x] / 255.0;
                    const auto currentPixel = pb.GetPixel(renderPoint + Point{x, y});

                    const auto scaledColour = Blend(currentPixel, colour, v);
                    pb.PutPixel(renderPoint + Point{x, y}, scaledColour);
                }
            }
            current.x += glyph.advance * font.GetScale();
        }
    }

    int GetTextWidth(Font& font, std::string_view text) {
        int width = 0;
        for (const auto ch : text) {
            const auto& glyph = font.GetGlyph(ch);
            width += glyph.advance * font.GetScale();
        }
        return width;
    }
}

