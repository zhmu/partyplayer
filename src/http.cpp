/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "http.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <optional>
#include <stdexcept>
#include <array>
#include <unistd.h>
#include <sstream>
#include <string_view>
#include "player.h"

namespace http {

static constexpr inline auto MaxRequestLength = 1024;
static constexpr inline std::string_view HeaderConnection("connection");
static constexpr inline std::string_view ValueKeepAlive("keep-alive");

namespace {
    auto TrimWhitespace(std::string_view sv)
    {
        sv.remove_prefix(std::min(sv.find_first_not_of(" "), sv.size()));
        sv.remove_suffix(std::min(sv.find_last_not_of(" "), std::size_t{0}));
        return sv;
    }

    bool equals_case_insensitive(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size()) return false;
        for(size_t n = 0; n < a.size(); ++n) {
            if (std::tolower(a[n]) != std::tolower(b[n])) return false;
        }
        return true;
    }

    // Parses a HTTP request to a pair of the location and the headers/content
    std::optional<std::pair<std::string_view, std::string_view>> ParseRequest(std::string_view request)
    {
        auto newline_index = request.find('\n');
        if (newline_index == std::string_view::npos) return {};
        const auto first_line = request.substr(0, newline_index - 1);
        const auto headers = request.substr(newline_index + 1);

        // First line should be "GET / HTTP/x.x"
        if (first_line.substr(0, 4) != "GET ") return {};
        const auto loc_http = first_line.substr(4);
        auto next_space_index = loc_http.find(" ");
        if (next_space_index == std::string_view::npos) return {};
        const auto location = loc_http.substr(0, next_space_index);
        const auto http = loc_http.substr(next_space_index + 1);
        if (http.substr(0, 7) != "HTTP/1.") return {};

        return make_pair(location, headers);
    }

    // Isolates all headers from request and calls callback(key, value)
    // Returns the content after the headers
    template<typename Func>
    std::optional<std::string_view> ProcessHeaders(std::string_view request, Func&& callback)
    {
        size_t index = 0;
        while(true) {
            auto index_next = request.find('\n', index);
            if (index_next == std::string_view::npos) {
                // Must end with a \n\n pair, so this is malformed
                return {};
            }

            const auto header = request.substr(index, index_next - index - 1);
            if (header.empty()) {
                // Empty header line, payload is next
                return request.substr(index);
            }
            const auto colon = header.find(':');
            if (colon == std::string_view::npos) return {}; // malformed
            const auto key = TrimWhitespace(header.substr(0, colon));
            const auto value = TrimWhitespace(header.substr(colon + 1));

            callback(key, value);
            index = index_next + 1;
        }
    }

    void SendHeader(const int fd, const int code, std::string_view reply)
    {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << code << " " << reply << "\r\n";
        const auto s = ss.str();
        write(fd, s.data(), s.size());
    }

    void SendReply(const int fd, const int code, std::string_view status, const std::string_view headers, const std::string_view payload)
    {
        SendHeader(fd, code, status);
        std::ostringstream ss;
        ss << "Content-Length: " << payload.size() << "\r\n";
        ss << headers;
        ss << "\r\n";
        const auto all_headers = ss.str();
        write(fd, all_headers.data(), all_headers.size());
        write(fd, payload.data(), payload.size());
    }

    void SendBadRequest(const int fd)
    {
        SendReply(fd, 400, "Bad Request", "", "");
    }

    void SendNotFound(const int fd)
    {
        SendReply(fd, 404, "Not Found", "", "");
    }

    void SendRedirect(const int fd, const std::string_view location)
    {
        std::stringstream ss;
        ss << "Location: " << location << "\r\n";
        SendReply(fd, 307, "Temporary Redirect", ss.str(), "");
    }

    void SendOK(const int fd, const std::string_view payload)
    {
        SendReply(fd, 200, "OK", "", payload);
    }

    template<typename Func>
    bool HandleHTTP(const int fd, const std::string_view request, Func&& callback)
    {
        auto req = ParseRequest(request);
        if (!req) {
            SendBadRequest(fd);
            return false;
        }
        auto [ location, headers ] = *req;

        auto keep_alive = false;
        const auto payload = ProcessHeaders(headers, [&](auto key, auto value) {
            // 'Connection': 'keep-alive'
            if (equals_case_insensitive(key, HeaderConnection))
                keep_alive = equals_case_insensitive(value, ValueKeepAlive);
        });
        if (!payload) return false;

        callback(fd, location, *payload);
        return keep_alive;
    }
}

Server::Server(player::Player& player, const PortNumber port)
    : player(player)
    , server_fd(socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0))
{
    struct CloseFd {
        int fd;
        ~CloseFd() { if(fd >= 0) close(fd); }
        void Cancel() { fd = -1; };
    } closer{server_fd};

    if (server_fd < 0) throw std::runtime_error("cannot create server socket");

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) < 0) throw std::runtime_error("cannot bind socket");
    if (listen(server_fd, 5) < 0) throw std::runtime_error("cannot listen socket");

    closer.Cancel();
}

Server::~Server()
{
    for(auto fd: client_fds)
        close(fd);
    close(server_fd);
}

void Server::Handle(std::chrono::microseconds timeout)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(server_fd, &fds);
    int max_fd = server_fd;
    for(auto fd: client_fds) {
        FD_SET(fd, &fds);
        max_fd = std::max(max_fd, fd);
    }

    struct timeval tv{};
    tv.tv_usec = timeout.count();
    int n = select(max_fd + 1, &fds, nullptr, nullptr, &tv);
    if (n < 0) {
        if (errno != EINTR)
            throw std::runtime_error("select() failed");
        return;
    }

    if (FD_ISSET(server_fd, &fds)) {
        if (int fd = accept4(server_fd, nullptr, nullptr, SOCK_CLOEXEC); fd >= 0) {
            client_fds.push_back(fd);
        }
        --n;
    }
    if (n == 0) return;

    for(auto it = client_fds.begin(); it != client_fds.end();) {
        int fd = *it;
        if (!FD_ISSET(fd, &fds)) { ++it; continue; }

        std::array<std::byte, MaxRequestLength> buffer;
        const auto num_bytes = read(fd, buffer.data(), buffer.size());
        if (num_bytes <= 0) {
            it = client_fds.erase(it);
            continue;
        }

        auto callback = [&](const int fd, std::string_view location, std::string_view payload) {
            if (location == "/") {
                std::stringstream ss;
                ss << "<html><head><title>Party Player</title></head><body>";
                ss << "Current track: <b>" << player.GetCurrentTrackInfo() << "</b><br/>\n";
                ss << "<a href=\"/next\">skip</a>\n";
                ss << "</body></html>";
                SendOK(fd, ss.str());
            } else if (location == "/next") {
                player.Skip();
                SendRedirect(fd, "/");
            } else {
                SendNotFound(fd);
            }
        };
        if (HandleHTTP(fd, {reinterpret_cast<char*>(buffer.data()), static_cast<size_t>(num_bytes)}, callback))
            ++it;
        else
            it = client_fds.erase(it);
    }
}

}
