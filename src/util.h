/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <vector>
#include <string_view>

namespace util {

std::vector<std::byte> ReadFile(const char* path);

class TextFile {
    std::vector<std::byte> buffer;
    std::vector<std::string_view> strings;
public:
    TextFile(std::vector<std::byte> input);
    const auto& GetStrings() const { return strings; }
};

}
