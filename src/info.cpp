/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "info.h"
#include <id3/tag.h>
#include <algorithm>

namespace info {

namespace {

uint16_t swap_bytes_u16(uint16_t v)
{
    return (v >> 8) | ((v & 0xff) << 8);
}

std::string unicode_to_string(unicode_t* buffer, size_t len)
{
    std::string s;
    for(size_t n = 0; n < len; ++n) {
        const auto ch = swap_bytes_u16(buffer[n]);
        if (ch > 0 && ch < 255 && std::isprint(static_cast<unsigned char>(ch))) {
            s += ch;
        }
    }
    return s;
}

std::string char_to_string(const char* buffer, size_t len)
{
    std::string s(buffer, len);
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isprint(ch);
    }), s.end());
    return s;
}

}

std::string GetTrackInfo(std::string_view path)
{
    ID3_Tag tag;
    tag.Link(std::string(path).c_str());

    auto getTag = [&](const ID3_FrameID fid) -> std::string {
        ID3_Frame* f = tag.Find(fid);
        if (f == nullptr) return {};
        char cbuffer[128] = {};
        const auto clen = f->Field(ID3FN_TEXT).Get(cbuffer, sizeof(cbuffer) - 1);
        if (clen > 0) return char_to_string(cbuffer, clen);

        unicode_t ubuffer[128] = {};
        const auto ulen = f->Field(ID3FN_TEXT).Get(ubuffer, sizeof(ubuffer) - 1);
        return unicode_to_string(ubuffer, ulen);
    };

    auto artist = getTag(ID3FID_LEADARTIST);
    auto title = getTag(ID3FID_TITLE);
    if (artist.empty()) artist = "?";
    if (title.empty()) title = "?";
    return artist + " / " + title;
}

}
