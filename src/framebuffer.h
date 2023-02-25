/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <cstdint>
#include "types.h"

struct PixelBuffer;

class FrameBuffer {
    int fd;
    uint32_t* memory;
    Size size;
public:
    FrameBuffer(const char* path);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    auto GetSize() const { return size; }
    void Render(const PixelBuffer& pb);
};
