/*-
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * Copyright (c) 2023 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include <vector>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <signal.h>
#include <utility>
#include "font.h"
#include "pixelbuffer.h"
#include "framebuffer.h"
#include "image.h"
#include "info.h"
#include "types.h"
#include "util.h"
#include "player.h"
#include "http.h"

namespace {

static constexpr inline auto SHOW_COPPERBARS = true;
static constexpr inline auto SHOW_LOGO = true;
static constexpr inline auto SHOW_STARFIELD = true;
static constexpr inline auto SHOW_CURRENT = true;
static constexpr inline auto SHOW_PREVIOUS = true;

enum class ScrollDirection {
    RightToLeft,
    LeftToRight,
};

struct Scroller
{
    font::Font& font;
    int speed{1};
    ScrollDirection direction;
    int x{0};
    std::string text;
    int width{0};

    void SetText(std::string sv)
    {
        text = std::move(sv);
        width = font::GetTextWidth(font, text);
        if (direction == ScrollDirection::RightToLeft) {
            x = width;
        } else /* direction == ScrollDirection::LeftToRight */ {
            x = -width;
        }
    }

    void Update(PixelBuffer& pb, const Colour& colour, int y)
    {
        font::DrawText(pb, font, { x, y }, colour, text);
        if (direction == ScrollDirection::RightToLeft) {
            x -= speed;
            if (x < -width)
                x = pb.GetSize().width;
        } else /* direction == ScrollDirection::LeftToRight */ {
            x += speed;
            if (x > pb.GetSize().width)
                x = -width;
        }
    }
};

class Copperbar
{
    const int half_height;
    const int top;
    const int bottom;
    const Colour from_colour;
    const Colour to_colour;
    int y;
    int direction{1};
public:
    Copperbar(const int height, const int top, const int bottom, const Colour& from_colour, const Colour& to_colour)
        : half_height(height / 2), top(top), bottom(bottom), from_colour(from_colour), to_colour(to_colour)
    {
        y = top;
        direction = 1;
    }

    void Advance(int steps = 1)
    {
        while (steps--) {
            y += direction;
            if (direction > 0) {
                if (y > bottom - half_height) {
                    direction = -direction;
                }
            } else /* direction < 0 */ {
                if (y < top - half_height) {
                    direction = -direction;
                }
            }
        }
    }

    void Update(PixelBuffer& pb)
    {
        Advance();
        for (int j = 0; j < half_height; ++j) {
            const auto c = Blend(from_colour, to_colour, static_cast<float>(j) / static_cast<float>(half_height));
            pb.Line({ 0, y - half_height + j }, { pb.GetSize().width, y - half_height + j }, c);
        }
        for (int j = 0; j < half_height; ++j) {
            const auto c = Blend(to_colour, from_colour, static_cast<float>(j) / static_cast<float>(half_height));
            pb.Line({ 0, y + j }, { pb.GetSize().width, y + j }, c);
        }
    }
};

void PlotLogo(PixelBuffer& pb, PixelBuffer& logo, int dest_x, int dest_y)
{
        for(int y = 0; y < logo.GetSize().height; ++y) {
            for (int x = 0; x < logo.GetSize().width; ++x) {
                auto p = &pb.buffer[(dest_y + y) * pb.GetSize().width + dest_x + x];
                auto source = &logo.buffer[y * logo.GetSize().width + x];
                if (*source != 0xffffffff)
                    *p = *source;
            }
        }
}

struct Star {
    Point position;
    int intensity{};
};

struct Starfield {
    const Rectangle rect;
    std::array<Star, 100> stars;

    std::uniform_int_distribution<int> x_dist;
    std::uniform_int_distribution<int> y_dist;
    std::uniform_int_distribution<int> intensity_dist;

    void ResetStar(std::mt19937& rng, Star& s)
    {
        s.position.x = rect.point.x + rect.size.width + x_dist(rng);
        s.position.y = rect.point.y + y_dist(rng);
        s.intensity = intensity_dist(rng);
    }

    Starfield(std::mt19937& rng, const Rectangle& r)
        : rect(r)
        , x_dist(0, 20)
        , y_dist(0, r.size.height)
        , intensity_dist(1, 10)
    {
        for(auto& star: stars) {
            ResetStar(rng, star);
        }
    }

    void Update(std::mt19937& rng, PixelBuffer& pb)
    {
        const Colour from{0, 0, 0};
        const Colour to{255, 255, 255};
        for(auto& star: stars) {
            pb.PutPixel(star.position, Blend(from, to, static_cast<float>(star.intensity - 1) / 9.0f));

            star.position.x -= star.intensity;
            if (star.position.x < 0)
                ResetStar(rng, star);
        }
    }
};

bool terminating = false;
bool child_attention = false;

void hook_signal(int signum, void(*handler)(int))
{
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigaction(signum, &sa, nullptr);
}

}

int main()
{
    hook_signal(SIGINT, [](int) { terminating = true; });
    hook_signal(SIGTERM, [](int) { terminating = true; });
    hook_signal(SIGCHLD, [](int) { child_attention = true; });

    std::random_device rd;
    std::mt19937 rng;
    rng.seed(rd());

    util::TextFile tf(util::ReadFile("../data/files.txt"));
    player::TrackPicker items(rng, std::move(tf), 0);

    player::Player player(std::move(items));

    http::Server server(player, 8000);

    FrameBuffer fb("/dev/fb0");
    std::cout << "framebuffer size " << fb.GetSize().width << " x " << fb.GetSize().height << '\n';
    PixelBuffer pb(fb.GetSize());

    font::Font main_font(util::ReadFile("../data/Roboto-Regular.ttf"), 70);
    font::Font thin_font(util::ReadFile("../data/Roboto-Thin.ttf"), 20);

    auto logo = image::Decode(util::ReadFile("../data/logo.png"));

    Scroller main_scroller(main_font);
    main_scroller.direction = ScrollDirection::RightToLeft;
    main_scroller.speed = 2;
    main_scroller.SetText("Starting up...");

    Scroller thin_scroller(thin_font);
    thin_scroller.direction = ScrollDirection::LeftToRight;
    thin_scroller.SetText("Hold your horses!");

    Copperbar redbar(20, 40, 120, {0, 0, 0}, {255, 0, 0});
    Copperbar greenbar(20, 40, 120, {0, 0, 0}, {0, 255, 0});
    Copperbar bluebar(20, 40, 120, {0, 0, 0}, {0, 0, 255});
    greenbar.Advance(20);
    bluebar.Advance(40);

    const auto logo_x = (pb.GetSize().width - logo.GetSize().width) / 2;
    const auto logo_y = 40 + (logo.GetSize().height / 2);

    Starfield starfield(rng, { 0, 0, pb.GetSize().width, pb.GetSize().height });

    child_attention = true;
    while(!terminating) {
        if (std::exchange(child_attention, false)) {
            player.OnChildTermination();

            main_scroller.SetText(std::string(player.GetCurrentTrackInfo()));
            if (!player.GetPreviousTrackInfo().empty()) {
                thin_scroller.SetText(std::string("Previous track: ") + std::string(player.GetPreviousTrackInfo()));
            }
        }

        pb.FilledRectangle({ {}, fb.GetSize() }, Colour{ 0, 0, 0 } );
        if constexpr (SHOW_STARFIELD) {
            starfield.Update(rng, pb);
        }

        if constexpr (SHOW_COPPERBARS) {
            redbar.Update(pb);
            greenbar.Update(pb);
            bluebar.Update(pb);
        }
        if constexpr (SHOW_LOGO) {
            PlotLogo(pb, logo, logo_x, logo_y);
        }

        if constexpr (SHOW_CURRENT) {
            main_scroller.Update(pb, { 255, 255, 255 }, 130);
        }

        if constexpr (SHOW_PREVIOUS) {
            thin_scroller.Update(pb, { 255, 255, 255 }, pb.GetSize().height - 20);
        }

        fb.Render(pb);
        server.Handle(std::chrono::milliseconds{ 10 });
    }

    return 0;
}
