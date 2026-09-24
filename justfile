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

# run tests (ctest)
test: build
    ctest --test-dir build --output-on-failure

# run main exe
run *args: build
    ./build/stencil "$@"

# debug main exe under gdb: just debug [args...]
debug *args: build
    gdb --args ./build/stencil "$@"


# format all sources in place (clang-format)
format:
    find src include test bench -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 | xargs -0r clang-format -i

# install binaries (cmake --install)
install: build
    cmake --install build

# remove the build directory
clean:
    rm -rf build compile_commands.json

