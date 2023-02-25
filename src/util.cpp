/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "util.h"
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>
#include <algorithm>

namespace util {

std::vector<std::byte> ReadFile(const char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        throw std::runtime_error("cannot open file");

    struct DerefClose {
        ~DerefClose() { close(fd); }
        int fd;
    } dc{fd};

    const auto file_size = lseek(fd, 0, SEEK_END);
    if (file_size == static_cast<off_t>(-1))
        throw std::runtime_error("cannot seek file");
    std::vector<std::byte> result(file_size);
    if (pread(fd, result.data(), result.size(), 0) != result.size())
        throw std::runtime_error("cannot read file");

    return result;
}

TextFile::TextFile(std::vector<std::byte> input)
    : buffer(std::move(input))
{
    auto current = buffer.begin();
    while(true) {
        auto next = std::find_if(current, buffer.end(), [](const auto b) {
            return b == static_cast<std::byte>('\n');
        });
        if (current != next) {
            strings.emplace_back(reinterpret_cast<const char*>(&*current), reinterpret_cast<const char*>(&*next));
        }
        if (next == buffer.end())
            break;
        current = next + 1;
    }
}

}
