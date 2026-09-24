#include <sched.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "modes.h"

using namespace std;

namespace {

// Linux /proc and /sys readers; each returns "" when the file or field is absent.
string first_line(const string& path) {
    ifstream file(path);
    string line;
    getline(file, line);
    return line;
}

string proc_field(const string& path, string_view key) {
    ifstream file(path);
    for (string line; getline(file, line);) {
        if (!line.starts_with(key))
            continue;
        const auto colon = line.find(':');
        const auto value =
            colon == string::npos ? string::npos : line.find_first_not_of(" \t", colon + 1);
        return value == string::npos ? "" : line.substr(value);
    }
    return "";
}

string or_unknown(string value) { return value.empty() ? "unknown" : value; }

string memory_total() {
    const string kib = proc_field("/proc/meminfo", "MemTotal"); // "65432100 kB"
    return kib.empty() ? "unknown" : format("{:.1f} GiB", stod(kib) / (1024.0 * 1024.0));
}

// Data and unified caches of one CPU; on the 9950X3D the two CCDs report different L3 sizes.
string cache_sizes(int cpu) {
    vector<string> caches;
    for (int index = 0;; ++index) {
        const string dir = format("/sys/devices/system/cpu/cpu{}/cache/index{}/", cpu, index);
        const string level = first_line(dir + "level");
        if (level.empty())
            break;
        const string type = first_line(dir + "type");
        if (type != "Instruction")
            caches.push_back(
                format("L{}{} {}", level, type == "Data" ? "d" : "", first_line(dir + "size")));
    }
    return or_unknown(caches | views::join_with("  "sv) | ranges::to<string>());
}

int allowed_cpu_count() {
    cpu_set_t mask;
    return sched_getaffinity(0, sizeof mask, &mask) == 0 ? CPU_COUNT(&mask) : -1;
}

} // namespace

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

    const int start_cpu = sched_getcpu();
    run_steps(); // warmup: page faults and cache state
    vector<double> step_times(reps);
    for (auto& t : step_times)
        t = run_steps();
    const int end_cpu = sched_getcpu();
    ranges::sort(step_times);

    const double median = step_times[reps / 2];
    const double p99 = step_times[static_cast<size_t>(ceil(0.99 * reps)) - 1];
    const double point_updates = static_cast<double>(nx * ny * nz * steps);
    constexpr double flops_per_point = 8.0;  // 6 adds + 2 muls
    constexpr double bytes_per_point = 24.0; // read u, write u_next, write-allocate on u_next

    // One paste-able block: provenance first, then results.
    println("== stencil-lab bench ==");
    println("date      {:%Y-%m-%d %H:%M:%S} UTC",
            chrono::floor<chrono::seconds>(chrono::system_clock::now()));
    println("host      {}", or_unknown(first_line("/proc/sys/kernel/hostname")));
    println("cpu       {}, {} logical", or_unknown(proc_field("/proc/cpuinfo", "model name")),
            thread::hardware_concurrency());
    println("ran on    cpu {} -> cpu {} (affinity: {} cpus)", start_cpu, end_cpu,
            allowed_cpu_count());
    println("caches    {} (cpu {})", cache_sizes(start_cpu), start_cpu);
    println("memory    {}", memory_total());
    println("kernel    {}", or_unknown(first_line("/proc/sys/kernel/osrelease")));
    println("governor  {}",
            or_unknown(first_line(
                format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_governor", start_cpu))));
    println("compiler  {}", STENCIL_COMPILER);
    println("build     {} [{}] commit {}", STENCIL_BUILD_TYPE, STENCIL_BUILD_FLAGS,
            STENCIL_GIT_COMMIT);
    println("solver    {}", solver.name());
    println("grid      {}x{}x{}, {} steps x {} reps", nx, ny, nz, steps, reps);
    println("time      median {:.4f} s   p99 {:.4f} s", median, p99);
    println("rate      {:.2f} GFLOP/s   {:.2f} GB/s (model)",
            point_updates * flops_per_point / median / 1e9,
            point_updates * bytes_per_point / median / 1e9);
    println("checksum  {:.17g}", solver.interior_sum());
}
