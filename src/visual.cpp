#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "modes.h"

namespace {

[[noreturn]] void throw_sdl_error(const char* what) {
    throw std::runtime_error(std::format("{}: {}", what, SDL_GetError()));
}

struct SdlSession {
    SdlSession() {
        if (!SDL_Init(SDL_INIT_VIDEO))
            throw_sdl_error("SDL_Init");
    }
    ~SdlSession() { SDL_Quit(); }
    SdlSession(const SdlSession&) = delete;
    SdlSession& operator=(const SdlSession&) = delete;
};

using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using RendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

constexpr std::size_t max_voxels_per_axis = 48;
// Brightness knobs: additive blending sums every voxel along a line of sight, so alpha must stay low.
constexpr float voxel_alpha = 0.2f;
constexpr double visible_threshold = 1e-3;
constexpr double tilt_radians = 0.45;
constexpr double spin_radians_per_second = 0.4;

SDL_FColor heat_color(double temperature) {
    // gamma < 1 keeps the cooling tail visible after the peak has dropped
    const float t = static_cast<float>(std::sqrt(std::clamp(temperature, 0.0, 1.0)));
    auto ramp = [t](float offset) { return std::clamp(3.0f * t - offset, 0.0f, 1.0f); };
    return {ramp(0.0f), ramp(1.0f), ramp(2.0f), voxel_alpha * t};
}

// Orthographic view: spin about the vertical (k) axis, then tilt toward the viewer.
struct Camera {
    float center_x, center_y, scale;
    double cos_yaw, sin_yaw;

    SDL_FPoint project(double x, double y, double z) const {
        const double spun_x = x * cos_yaw - y * sin_yaw;
        const double spun_y = x * sin_yaw + y * cos_yaw;
        const double screen_up = z * std::cos(tilt_radians) + spun_y * std::sin(tilt_radians);
        return {center_x + scale * static_cast<float>(spun_x),
                center_y - scale * static_cast<float>(screen_up)};
    }
};

} // namespace

void scr::run_visual(const LabContext& ctx) {
    SdlSession sdl;
    SDL_Window* raw_window = nullptr;
    SDL_Renderer* raw_renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("stencil-lab", 900, 900, SDL_WINDOW_RESIZABLE, &raw_window,
                                     &raw_renderer))
        throw_sdl_error("SDL_CreateWindowAndRenderer");
    WindowPtr window(raw_window, SDL_DestroyWindow);
    RendererPtr renderer(raw_renderer, SDL_DestroyRenderer);
    SDL_SetRenderVSync(renderer.get(), 1);

    ctx.solver.init();
    fill_hot_cube(ctx);

    const double longest_axis = static_cast<double>(std::max({ctx.nx, ctx.ny, ctx.nz}));
    const std::size_t stride =
        std::max<std::size_t>(1, static_cast<std::size_t>(longest_axis) / max_voxels_per_axis);
    // grid index -> [-0.5, 0.5] on the longest axis, keeping aspect ratio
    auto normalized = [&](std::size_t index, std::size_t n) {
        return (static_cast<double>(index) - 0.5 * static_cast<double>(n + 1)) / longest_axis;
    };

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    std::size_t steps_taken = 0;

    for (bool running = true; running;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
                running = false;
        }

        for (std::size_t s = 0; s < ctx.steps; ++s)
            ctx.solver.step();
        steps_taken += ctx.steps;

        int width = 0, height = 0;
        SDL_GetRenderOutputSize(renderer.get(), &width, &height);
        const double yaw = spin_radians_per_second * static_cast<double>(SDL_GetTicks()) / 1000.0;
        const Camera camera{0.5f * width, 0.5f * height, 0.55f * std::min(width, height), std::cos(yaw),
                            std::sin(yaw)};
        const float half_size = 0.5f * camera.scale * static_cast<float>(stride / longest_axis);

        vertices.clear();
        indices.clear();
        double peak = 0.0;
        for (std::size_t k = 1; k <= ctx.nz; k += stride)
            for (std::size_t j = 1; j <= ctx.ny; j += stride)
                for (std::size_t i = 1; i <= ctx.nx; i += stride) {
                    const double temperature = ctx.solver.get(i, j, k);
                    peak = std::max(peak, temperature);
                    if (temperature < visible_threshold)
                        continue;
                    const SDL_FPoint p = camera.project(normalized(i, ctx.nx), normalized(j, ctx.ny),
                                                        normalized(k, ctx.nz));
                    const SDL_FColor color = heat_color(temperature);
                    const int first = static_cast<int>(vertices.size());
                    vertices.push_back({{p.x - half_size, p.y - half_size}, color, {}});
                    vertices.push_back({{p.x + half_size, p.y - half_size}, color, {}});
                    vertices.push_back({{p.x + half_size, p.y + half_size}, color, {}});
                    vertices.push_back({{p.x - half_size, p.y + half_size}, color, {}});
                    indices.insert(indices.end(),
                                   {first, first + 1, first + 2, first, first + 2, first + 3});
                }

        SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
        SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_NONE);
        SDL_RenderClear(renderer.get());

        // untextured geometry blends with the draw blend mode; additive means no depth sort
        SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_ADD);
        if (!indices.empty() &&
            !SDL_RenderGeometry(renderer.get(), nullptr, vertices.data(), static_cast<int>(vertices.size()),
                                indices.data(), static_cast<int>(indices.size())))
            throw_sdl_error("SDL_RenderGeometry");

        SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer.get(), 90, 90, 110, 255);
        // corner bits 0/1/2 pick the -/+ face on x/y/z; box edges join corners differing in one bit
        auto box_corner = [&](int bits) {
            auto face = [&](int bit, std::size_t n) { return (bits & bit ? 0.5 : -0.5) * n / longest_axis; };
            return camera.project(face(1, ctx.nx), face(2, ctx.ny), face(4, ctx.nz));
        };
        for (int corner = 0; corner < 8; ++corner)
            for (int bit = 1; bit < 8; bit <<= 1)
                if (!(corner & bit)) {
                    const SDL_FPoint from = box_corner(corner);
                    const SDL_FPoint to = box_corner(corner | bit);
                    SDL_RenderLine(renderer.get(), from.x, from.y, to.x, to.y);
                }

        SDL_SetRenderDrawColor(renderer.get(), 220, 220, 220, 255);
        const std::string status = std::format("{}x{}x{}  step {}  peak {:.4f}  (esc quits)", ctx.nx,
                                               ctx.ny, ctx.nz, steps_taken, peak);
        SDL_RenderDebugText(renderer.get(), 10.0f, 10.0f, status.c_str());
        SDL_RenderPresent(renderer.get());
    }
}
