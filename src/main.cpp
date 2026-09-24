#include <cstdlib>
#include <exception>
#include <print>
#include <string_view>

#include "modes.h"
#include "stencil.h"

using namespace std;

int main(int argc, char** argv) {
    const string_view mode = argc > 1 ? argv[1] : "bench";
    const bool visual = mode == "visual";
    if (!visual && mode != "bench") {
        println(stderr, "usage: {} [bench|visual] [nx ny nz steps reps]  (all positive integers)", argv[0]);
        return 1;
    }

    const auto arg_or = [&](int position, size_t fallback) {
        return argc > position ? strtoull(argv[position], nullptr, 10) : fallback;
    };
    // visual: steps is steps per frame, reps is unused; the grid is small enough to redraw every frame
    const size_t default_n = visual ? 64 : 512;
    const size_t nx = arg_or(2, default_n);
    const size_t ny = arg_or(3, default_n);
    const size_t nz = arg_or(4, default_n);
    const size_t steps = arg_or(5, visual ? 4 : 10);
    const size_t reps = arg_or(6, 10);

    if (nx == 0 || ny == 0 || nz == 0 || steps == 0 || reps == 0) {
        println(stderr, "usage: {} [bench|visual] [nx ny nz steps reps]  (all positive integers)", argv[0]);
        return 1;
    }

    scr::NaiveStencilSolver solver(nx, ny, nz);
    scr::LabContext ctx{
        .solver = solver,
        .nx = nx,
        .ny = ny,
        .nz = nz,
        .steps = steps,
        .reps = reps,
    };

    try {
        if (visual)
            scr::run_visual(ctx);
        else
            scr::run_benchmark(ctx);
    } catch (const exception& e) {
        println(stderr, "{} mode failed: {}", mode, e.what());
        return 1;
    }
    return 0;
}
