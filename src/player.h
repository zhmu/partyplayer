/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <random>
#include <string_view>
#include "util.h"

namespace player {

class TrackPicker
{
    util::TextFile text;
    std::mt19937& rng;
    std::uniform_int_distribution<size_t> item_dist;
    size_t step{};

public:
    TrackPicker(std::mt19937& rng, util::TextFile tf, size_t step);

    std::string_view RetrieveNextItem();

    TrackPicker(const TrackPicker&) = delete;
    TrackPicker& operator=(const TrackPicker&) = delete;
    TrackPicker(TrackPicker&&) = default;
};

class Player
{
    TrackPicker picker;
    std::string_view current;
    std::string prev_track_info;
    std::string track_info;
    pid_t child_pid{-1};

public:
    Player(TrackPicker picker);

    std::string_view GetCurrentTrackInfo() const { return track_info; }
    std::string_view GetPreviousTrackInfo() const { return prev_track_info; }

    void Next();
    void Skip();
    void OnChildTermination();
};

}
