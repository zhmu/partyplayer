add_executable(partyplayer main.cpp font.cpp pixelbuffer.cpp framebuffer.cpp image.cpp util.cpp info.cpp player.cpp http.cpp)
target_link_libraries(partyplayer PRIVATE id3 spdlog::spdlog)

