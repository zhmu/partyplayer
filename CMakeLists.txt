cmake_minimum_required(VERSION 3.8)
project(partyplayer C CXX)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(3rdparty/spdlog-1.11.0)
add_subdirectory(src)
