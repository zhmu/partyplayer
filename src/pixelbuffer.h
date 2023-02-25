/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <memory>
#include "types.h"

struct PixelBuffer {
    const Size size;
    std::unique_ptr<PixelValue[]> buffer;

    explicit PixelBuffer(Size size);
    void FilledRectangle(const struct Rectangle& r, const Colour& colour);
    const Size& GetSize() const { return size; }

    Colour GetPixel(const Point& point) const
    {
        if (!In(size, point))
            return {};
        return FromPixelValue(buffer[point.y * size.width + point.x]);
    }

    void PutPixel(const Point& point, const Colour& c)
    {
        if (In(size, point))
            buffer[point.y * size.width + point.x] = c;
    }

    void Line(const Point& from, const Point& to, const Colour& colour);
    void Rectangle(const Rectangle& r, const Colour& colour);
};
