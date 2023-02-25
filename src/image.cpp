/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "image.h"
#include "pixelbuffer.h"
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "../3rdparty/stb/stb_image.h"

namespace image {
    PixelBuffer Decode(const std::vector<std::byte>& buffer)
    {
        int width, height, channels_in_file;
        auto image = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer.data()), buffer.size(), &width, &height, &channels_in_file, 4);
        if (image == NULL)
            throw std::runtime_error("unable to decode image");

        struct DerefFree {
            stbi_uc* image;
            ~DerefFree() { stbi_image_free(image); }
        } df(image);

        PixelBuffer result(Size{ width, height });
        for(unsigned int y = 0; y < height; ++y) {
            for(unsigned int x = 0; x < width; ++x) {
                auto r = image[(y * width + x) * 4 + 0];
                auto g = image[(y * width + x) * 4 + 1];
                auto b = image[(y * width + x) * 4 + 2];
                auto a = image[(y * width + x) * 4 + 3];
                result.buffer[y * width + x] = static_cast<PixelValue>(Colour{b, g, r, a});
            }
        }
        return result;
    }
}
