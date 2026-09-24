# stencil-lab

A 3D 7-point Jacobi stencil (heat diffusion on a regular grid), built up level by level
with a shared measurement harness.

## Requirements

- `g++-16` (set in `CMakeLists.txt`), CMake ≥ 3.20, Ninja, [`just`](https://github.com/casey/just)
- git + network on first configure: SDL3 isn't packaged for Ubuntu 24.04, so CMake fetches and
  builds a pinned release (~1 min, once per build directory). SDL's X11/Wayland dev headers
  need to be installed.

## Build and test

```sh
just build   # configure + compile
just test    # correctness tests (constant field, exact sine-mode decay)
just clean   # remove build/ and build-release/
```

`build/` is **Debug (`-O0`)**, for tests and gdb. Benchmark numbers from it are meaningless.
The `-release` recipes build **RelWithDebInfo** (`-O2 -g`, frame pointers for perf) into a
separate `build-release/`:

```sh
just build-release
just run-release bench 512 512 512
```

## Run

```sh
just run [bench|visual] [nx ny nz steps reps]           # Debug build
just run-release [bench|visual] [nx ny nz steps reps]   # RelWithDebInfo build
```

The mode defaults to `bench`. All numbers must be positive integers. Arguments go by position,
so to change `steps` you have to give `nx ny nz` too.

| Argument     | `bench`                        | `visual`                    |
| ------------ | ------------------------------ | --------------------------- |
| `nx ny nz`   | interior grid size (512³)      | interior grid size (64³)    |
| `steps`      | timesteps per timed rep (10)   | timesteps per frame (4)     |
| `reps`       | timed reps after 1 warmup (10) | unused                      |

### bench

Runs as fast as possible from the sine-mode initial field and reports:

```
grid 512x512x512, 10 steps x 10 reps
median 1.2345 s   p99 1.3456 s
... GFLOP/s   ... GB/s (model)
checksum ...
```

- **median / p99**: wall time of one rep (`steps` timesteps), over `reps` runs.
- **GFLOP/s**: 8 FLOPs per point update.
- **GB/s (model)**: 24 bytes per point update (read `u`, write `u_next`, write-allocate).
  This is a lower bound on traffic, so the true DRAM bandwidth can be higher.
- **checksum**: sum of the final interior. Compare it across levels with the same arguments.

Keep grids well past L3 (128 MB on the 9950X3D) for DRAM-bound numbers: 512³ is ~1 GB per
buffer. Small grids measure cache bandwidth instead.

```sh
just run-release bench                    # 512³ defaults
just run-release bench 256 256 256 20 15  # smaller grid, more steps and reps
```

### visual

Opens an SDL3 window showing a hot cube diffusing out through cold (zero) boundaries, drawn
as a rotating additive point cloud. The status line shows the step count and peak
temperature. Esc or closing the window quits.

```sh
just run visual              # 64³, 4 steps per frame
just run visual 96 64 48 8   # non-cubic grid, faster simulated time
```

Grids larger than 48 per axis are subsampled for drawing. The simulation itself runs at full
resolution, so large grids slow the frame rate.

## Layout

```
include/stencil.h   solver interface, naive solver, initial conditions
include/modes.h     mode entry points
src/main.cpp        argument parsing, mode dispatch
src/benchmark.cpp   timing harness
src/visual.cpp      SDL3 visualization
test/               correctness tests, one executable per file
```
