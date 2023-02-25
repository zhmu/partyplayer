/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <chrono>
#include <vector>

namespace player { class Player; }

namespace http {

using PortNumber = unsigned int;
using FileDescriptor = int;

class Server {
    player::Player& player;
    const FileDescriptor server_fd;
    std::vector<FileDescriptor> client_fds;
public:
    Server(player::Player& player, const PortNumber port);
    ~Server();

    void Handle(std::chrono::microseconds timeout);
};

}
