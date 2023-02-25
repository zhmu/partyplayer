/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "framebuffer.h"
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include "pixelbuffer.h"

FrameBuffer::FrameBuffer(const char* path)
{
    fd = open(path, O_RDWR);
    if (fd < 0)
        throw std::runtime_error("cannot open framebuffer");

    struct fb_var_screeninfo si;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &si) < 0)
        throw std::runtime_error("cannot query framebuffer info");

    if (si.bits_per_pixel != 32)
        throw std::runtime_error("unsupported bpp");
    size.width = si.xres;
    size.height = si.yres;

    auto fb = mmap(NULL, size.width * size.height * 4, PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED)
        throw std::runtime_error("unable to map framebuffer");
    memory = reinterpret_cast<uint32_t*>(fb);
}

FrameBuffer::~FrameBuffer()
{
    munmap(memory, size.width * size.height * 4);
    close(fd);
}

void FrameBuffer::Render(const PixelBuffer& pb)
{
    memcpy(memory, pb.buffer.get(), size.width * size.height * 4);
}
