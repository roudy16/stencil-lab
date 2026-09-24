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

Runs as fast as possible from the sine-mode initial field and prints one block, meant to be
pasted into a log as-is:

```
== stencil-lab bench ==
date      2026-09-24 20:42:44 UTC
host      pop-os
cpu       AMD Ryzen 9 9950X3D 16-Core Processor, 32 logical
ran on    cpu 11 -> cpu 25 (affinity: 32 cpus)
caches    L1d 48K  L2 1024K  L3 32768K (cpu 11)
memory    249.3 GiB
kernel    7.1.5-76070105-generic
governor  powersave
compiler  GNU 16.0.1
build     RelWithDebInfo [-O2 -g -DNDEBUG] commit 1a7fa01-dirty
solver    L0 naive
grid      128x128x128, 5 steps x 5 reps
time      median 0.0100 s   p99 0.0109 s
rate      8.37 GFLOP/s   25.12 GB/s (model)
checksum  550107.0359960401
```

- **ran on**: the CPU at the start and end of the timed runs, and how many CPUs the process
  was allowed on. Differing CPUs mean the thread migrated mid-run.
- **caches**: for the starting CPU. The machine info is read from Linux `/proc` and `/sys`,
  and anything missing prints `unknown`.
- **build / commit**: captured at configure time. `-dirty` means uncommitted changes were
  built.
- **time**: median and p99 wall time of one rep (`steps` timesteps), over `reps` runs.
- **rate**: 8 FLOPs per point update. The GB/s figure is a model of 24 bytes per point update
  (read `u`, write `u_next`, write-allocate). It's a lower bound on traffic, so the true DRAM
  bandwidth can be higher.
- **checksum**: sum of the final interior. Compare it across levels with the same arguments.

Keep grids well past L3 (128 MB on the 9950X3D) for DRAM-bound numbers: 512³ is ~1 GB per
buffer. Small grids measure cache bandwidth instead.

**Pin the run to one CPU** for comparable numbers. On the 9950X3D the two CCDs have different
L3 sizes (cpus 0–7 and 16–23 have 96 MB; 8–15 and 24–31 have 32 MB), and an unpinned run can
migrate between them:

```sh
just build-release
taskset -c 2 ./build-release/stencil bench                    # 512³ defaults, pinned
taskset -c 2 ./build-release/stencil bench 256 256 256 20 15  # smaller grid, more steps and reps
```

### stream

Measures the machine's memory-bandwidth ceiling with McCalpin's STREAM, run once per machine.
The first run downloads `stream.c` into `build/stream/`. It prints a paste-able block with one
single-thread run pinned to `cpu` and one run with a thread per physical core.

```sh
just stream               # 1T on cpu 0, 200M-double (1.6 GB) arrays
just stream 27            # pin the 1T run to the CPU your bench ran on
just stream 2 800000000   # bigger arrays: needed once total L3 exceeds ~400 MB (4x rule)
```

STREAM's rates leave out write-allocate traffic, but the stencil's 24 B/point model includes
it. Multiply Triad by 4/3 before comparing it with the bench's GB/s.

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
