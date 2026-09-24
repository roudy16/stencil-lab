#include <algorithm>
#include <chrono>
#include <cmath>
#include <print>
#include <vector>

#include "modes.h"

using namespace std;

void scr::run_benchmark(const LabContext& ctx) {
    auto& solver = ctx.solver;
    const size_t nx = ctx.nx;
    const size_t ny = ctx.ny;
    const size_t nz = ctx.nz;
    const size_t steps = ctx.steps;
    const size_t reps = ctx.reps;

    ctx.solver.init();
    fill_sine_mode(ctx);

    auto run_steps = [&] {
        const auto start = chrono::steady_clock::now();
        for (size_t s = 0; s < steps; ++s)
            solver.step();
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    };

    run_steps(); // warmup: page faults and cache state
    vector<double> step_times(reps);
    for (auto& t : step_times)
        t = run_steps();
    ranges::sort(step_times);

    const double median = step_times[reps / 2];
    const double p99 = step_times[static_cast<size_t>(ceil(0.99 * reps)) - 1];
    const double point_updates = static_cast<double>(nx * ny * nz * steps);
    constexpr double flops_per_point = 8.0;  // 6 adds + 2 muls
    constexpr double bytes_per_point = 24.0; // read u, write u_next, write-allocate on u_next

    println("grid {}x{}x{}, {} steps x {} reps", nx, ny, nz, steps, reps);
    println("median {:.4f} s   p99 {:.4f} s", median, p99);
    println("{:.2f} GFLOP/s   {:.2f} GB/s (model)", point_updates * flops_per_point / median / 1e9,
            point_updates * bytes_per_point / median / 1e9);
    println("checksum {:.17g}", solver.interior_sum());
}
