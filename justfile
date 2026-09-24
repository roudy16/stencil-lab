# stencil-lab

set positional-arguments

# list recipes
default:
    @just --list

# configure the build (also refreshes compile_commands.json for clangd)
setup:
    cmake -B build -G Ninja
    ln -sf build/compile_commands.json compile_commands.json

# compile everything
build: setup
    cmake --build build

# compile the RelWithDebInfo build (-O2 -g, frame pointers) into build-release/
build-release:
    cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build build-release

# run tests (ctest)
test: build
    ctest --test-dir build --output-on-failure

# run main exe
run *args: build
    ./build/stencil "$@"

# run main exe from the RelWithDebInfo build; use this for benchmark numbers
run-release *args: build-release
    ./build-release/stencil "$@"

# debug main exe under gdb: just debug [args...]
debug *args: build
    gdb --args ./build/stencil "$@"

# STREAM memory-bandwidth ceiling: 1 thread pinned to `cpu`, then one thread per core.
# Arrays default to 200M doubles (1.6 GB each); keep each >= 4x total L3.
stream cpu="0" array_size="200000000":
    #!/usr/bin/env bash
    set -euo pipefail
    dir=build/stream
    mkdir -p "$dir"
    [[ -f "$dir/stream.c" ]] || curl -fsSL -o "$dir/stream.c" https://www.cs.virginia.edu/stream/FTP/Code/stream.c
    cc=$(command -v gcc-16 || command -v gcc)
    flags="-O3 -march=native -fopenmp -mcmodel=medium -DSTREAM_ARRAY_SIZE={{array_size}} -DNTIMES=20"
    "$cc" $flags -w "$dir/stream.c" -o "$dir/stream"
    cores=$(lscpu -p=CORE | grep -v '^#' | sort -u | wc -l)
    echo "== STREAM =="
    echo "date      $(date -u '+%Y-%m-%d %H:%M:%S') UTC"
    echo "host      $(cat /proc/sys/kernel/hostname)"
    echo "cpu       $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //'), $(nproc --all) logical, $cores cores"
    echo "memory    $(awk '/MemTotal/ {printf "%.1f GiB", $2 / 1048576}' /proc/meminfo)"
    echo "governor  $(cat /sys/devices/system/cpu/cpu{{cpu}}/cpufreq/scaling_governor 2>/dev/null || echo unknown)"
    echo "compiler  $(basename "$cc") $("$cc" -dumpfullversion) $flags"
    echo "stream    $(grep -m1 -o '[$]Revision: [0-9.]*' "$dir/stream.c" | cut -c2-), rates are best-of-20 MB/s, write-allocate not counted"
    run() {
        local label=$1; shift
        env "$@" "$dir/stream" | awk -v label="$label" '
            /^(Copy|Triad):/ { rate[$1] = $2 }
            /Solution Validates/ { ok = 1 }
            END { printf "%-22s Copy %9s   Triad %9s   %s\n", label, rate["Copy:"], rate["Triad:"], ok ? "validates" : "FAILED VALIDATION" }'
    }
    run "1T cpu {{cpu}}" OMP_NUM_THREADS=1 taskset -c {{cpu}}
    run "${cores}T one per core" OMP_NUM_THREADS="$cores" OMP_PLACES=cores OMP_PROC_BIND=spread

# format all sources in place (clang-format)
format:
    find src include test bench -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 | xargs -0r clang-format -i

# install binaries (cmake --install)
install: build
    cmake --install build

# remove the build directories
clean:
    rm -rf build build-release compile_commands.json

