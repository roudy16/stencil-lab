#include <cmath>
#include <cstddef>
#include <print>

#include "stencil.h"

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::println(stderr, "FAIL: {}", what);
        ++failures;
    }
}

// Unequal dims so a swapped index or stride can't hide behind symmetry.
constexpr std::size_t nx = 30, ny = 40, nz = 50;

void constant_field_is_steady() {
    scr::NaiveStencilSolver solver(nx, ny, nz);
    solver.init();
    for (std::size_t k = 0; k <= nz + 1; ++k)
        for (std::size_t j = 0; j <= ny + 1; ++j)
            for (std::size_t i = 0; i <= nx + 1; ++i)
                solver.set(i, j, k, 3.0);
    for (int s = 0; s < 5; ++s)
        solver.step();

    double max_error = 0.0;
    for (std::size_t k = 1; k <= nz; ++k)
        for (std::size_t j = 1; j <= ny; ++j)
            for (std::size_t i = 1; i <= nx; ++i)
                max_error = std::fmax(max_error, std::fabs(solver.get(i, j, k) - 3.0));
    check(max_error < 1e-13, "constant field stays constant");
}

void sine_mode_decays_exactly() {
    constexpr int steps = 20;
    constexpr int reps = 1;
    scr::NaiveStencilSolver solver(nx, ny, nz);
    scr::LabContext ctx{
        .solver = solver,
        .nx = nx,
        .ny = ny,
        .nz = nz,
        .steps = steps,
        .reps = reps,
    };
    solver.init();
    scr::fill_sine_mode(ctx);

    scr::NaiveStencilSolver initial(nx, ny, nz);
    scr::LabContext ctx_init{
        .solver = initial,
        .nx = nx,
        .ny = ny,
        .nz = nz,
        .steps = steps,
        .reps = reps,
    };
    initial.init();
    scr::fill_sine_mode(ctx_init);

    for (int s = 0; s < steps; ++s)
        solver.step();

    const double decay = std::pow(scr::sine_mode_decay(nx, ny, nz, solver.r), steps);
    double max_error = 0.0;
    for (std::size_t k = 1; k <= nz; ++k)
        for (std::size_t j = 1; j <= ny; ++j)
            for (std::size_t i = 1; i <= nx; ++i)
                max_error = std::fmax(
                    max_error, std::fabs(solver.get(i, j, k) - decay * initial.get(i, j, k)));
    check(max_error < 1e-12, "sine mode decays by lambda^T");
}

} // namespace

int main() {
    constant_field_is_steady();
    sine_mode_decays_exactly();
    if (failures == 0)
        std::println("all passed");
    return failures == 0 ? 0 : 1;
}
