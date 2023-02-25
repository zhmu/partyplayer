/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <vector>

struct PixelBuffer;

namespace image {
    PixelBuffer Decode(const std::vector<std::byte>& buffer);
}
