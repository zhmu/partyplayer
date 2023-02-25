/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "player.h"
#include <array>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "info.h"
#include "spdlog/spdlog.h"

namespace player {

TrackPicker::TrackPicker(std::mt19937& rng, util::TextFile tf, size_t step)
    : rng(rng)
    , text(std::move(tf))
    , item_dist(0, text.GetStrings().size())
{
    while(step--)
        (void)RetrieveNextItem();
}

std::string_view TrackPicker::RetrieveNextItem()
{
    const auto index = item_dist(rng);
    ++step;
    return text.GetStrings()[index];
}

Player::Player(TrackPicker picker) : picker(std::move(picker)) { }

void Player::Next()
{
    current = picker.RetrieveNextItem();
    if (current.empty()) return;

    prev_track_info = std::move(track_info);
    track_info = info::GetTrackInfo(current);

    spdlog::info("Playing '{}'", current);

    pid_t p = fork();
    if (p == 0) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        std::string executable("mplayer");
        std::string arg0("-really-quiet");
        std::string arg1(current);
        std::array<char*, 4> args{
            executable.data(),
            arg0.data(),
            arg1.data(),
            nullptr
        };
        execvp(executable.data(), &args[0]);
        exit(EXIT_FAILURE);
    }
    child_pid = p;
}

void Player::Skip()
{
    spdlog::info("Skipping track");
    kill(child_pid, SIGTERM);
    waitpid(0, NULL, 0);
}

void Player::OnChildTermination()
{
    waitpid(0, NULL, 0);
    Next();
}

}
