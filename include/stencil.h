#ifndef SCR_STENCIL_H
#define SCR_STENCIL_H

#include <cmath>
#include <cstddef>
#include <memory>
#include <numbers>
#include <string_view>
#include <utility>

namespace scr {

class StencilSolver {
public:
    using size_type = std::size_t;

    virtual ~StencilSolver() = default;
    virtual std::string_view name() const = 0;
    virtual void init() = 0;
    virtual void step() = 0;
    virtual void set(size_type i, size_type j, size_type k, double value) = 0;
    virtual double get(size_type i, size_type j, size_type k) const = 0;
    virtual double interior_sum() const = 0;
};

struct LabContext {
    scr::StencilSolver& solver;
    const size_t nx;
    const size_t ny;
    const size_t nz;
    const size_t steps;
    const size_t reps;
};

// Grid is (nx+2)*(ny+2)*(nz+2) with a one-cell halo holding fixed boundary values;
// i/j/k run 0..n+1, only 1..n is updated. x is the contiguous axis.
class NaiveStencilSolver final : public StencilSolver {
public:
    using size_type = std::size_t;

    const size_type nx;
    const size_type ny;
    const size_type nz;
    const double r;

    NaiveStencilSolver(size_type nx, size_type ny, size_type nz, double r = 0.125) noexcept
        : nx(nx), ny(ny), nz(nz), r(r) {}

    std::string_view name() const override { return "L0 naive"; }

    void init() override {
        const size_type total_points = (nx + 2) * (ny + 2) * (nz + 2);
        current = std::make_unique<double[]>(total_points);
        next = std::make_unique<double[]>(total_points);
    }

    void step() override {
        const size_type y_stride = nx + 2;
        const size_type z_stride = (nx + 2) * (ny + 2);
        const double center_weight = 1.0 - 6.0 * r;
        const double* u = current.get();
        double* u_next = next.get();

        for (size_type k = 1; k <= nz; ++k) {
            for (size_type j = 1; j <= ny; ++j) {
                for (size_type i = 1; i <= nx; ++i) {
                    const size_type c = index(i, j, k);
                    const double neighbor_sum = u[c - 1] + u[c + 1] + u[c - y_stride] +
                                                u[c + y_stride] + u[c - z_stride] + u[c + z_stride];
                    u_next[c] = center_weight * u[c] + r * neighbor_sum;
                }
            }
        }
        std::swap(current, next);
    }

    // Writes both buffers so halo values survive the swap.
    void set(size_type i, size_type j, size_type k, double value) override {
        const size_type c = index(i, j, k);
        current[c] = value;
        next[c] = value;
    }

    double get(size_type i, size_type j, size_type k) const override {
        return current[index(i, j, k)];
    }

    double interior_sum() const override {
        double sum = 0.0;
        for (size_type k = 1; k <= nz; ++k)
            for (size_type j = 1; j <= ny; ++j)
                for (size_type i = 1; i <= nx; ++i)
                    sum += get(i, j, k);
        return sum;
    }

private:
    std::unique_ptr<double[]> current;
    std::unique_ptr<double[]> next;

    constexpr size_type index(const size_type i, const size_type j, const size_type k) const {
        return (k * (ny + 2) + j) * (nx + 2) + i;
    }
};

class NaiveStencilSolverBadLoop final : public StencilSolver {
public:
    using size_type = std::size_t;

    const size_type nx;
    const size_type ny;
    const size_type nz;
    const double r;

    NaiveStencilSolverBadLoop(size_type nx, size_type ny, size_type nz, double r = 0.125) noexcept
        : nx(nx), ny(ny), nz(nz), r(r) {}

    std::string_view name() const override { return "L0 naive - bad loop"; }

    void init() override {
        const size_type total_points = (nx + 2) * (ny + 2) * (nz + 2);
        current = std::make_unique<double[]>(total_points);
        next = std::make_unique<double[]>(total_points);
    }

    // NOTE: bad on purpose
    void step() override {
        const size_type y_stride = nx + 2;
        const size_type z_stride = (nx + 2) * (ny + 2);
        const double center_weight = 1.0 - 6.0 * r;
        const double* u = current.get();
        double* u_next = next.get();

        for (size_type i = 1; i <= nx; ++i) {
            for (size_type j = 1; j <= ny; ++j) {
                for (size_type k = 1; k <= nz; ++k) {
                    const size_type c = index(i, j, k);
                    const double neighbor_sum = u[c - 1] + u[c + 1] + u[c - y_stride] +
                                                u[c + y_stride] + u[c - z_stride] + u[c + z_stride];
                    u_next[c] = center_weight * u[c] + r * neighbor_sum;
                }
            }
        }
        std::swap(current, next);
    }

    // Writes both buffers so halo values survive the swap.
    void set(size_type i, size_type j, size_type k, double value) override {
        const size_type c = index(i, j, k);
        current[c] = value;
        next[c] = value;
    }

    double get(size_type i, size_type j, size_type k) const override {
        return current[index(i, j, k)];
    }

    double interior_sum() const override {
        double sum = 0.0;
        for (size_type k = 1; k <= nz; ++k)
            for (size_type j = 1; j <= ny; ++j)
                for (size_type i = 1; i <= nx; ++i)
                    sum += get(i, j, k);
        return sum;
    }

private:
    std::unique_ptr<double[]> current;
    std::unique_ptr<double[]> next;

    constexpr size_type index(const size_type i, const size_type j, const size_type k) const {
        return (k * (ny + 2) + j) * (nx + 2) + i;
    }
};

// Lowest sine mode with zero boundaries: an exact eigenvector of the discrete update.
inline void fill_sine_mode(const LabContext& ctx) {
    using std::numbers::pi;
    for (std::size_t k = 1; k <= ctx.nz; ++k)
        for (std::size_t j = 1; j <= ctx.ny; ++j)
            for (std::size_t i = 1; i <= ctx.nx; ++i)
                ctx.solver.set(i, j, k,
                               std::sin(pi * i / (ctx.nx + 1)) * std::sin(pi * j / (ctx.ny + 1)) *
                                   std::sin(pi * k / (ctx.nz + 1)));
}

// Centered cube at 1.0 in a zero field with zero boundaries; spreads, then drains out the faces.
inline void fill_hot_cube(const LabContext& ctx) {
    auto is_hot = [](std::size_t index, std::size_t n) {
        return index > 3 * n / 8 && index <= 5 * n / 8;
    };
    for (std::size_t k = 1; k <= ctx.nz; ++k)
        for (std::size_t j = 1; j <= ctx.ny; ++j)
            for (std::size_t i = 1; i <= ctx.nx; ++i)
                ctx.solver.set(i, j, k,
                               is_hot(i, ctx.nx) && is_hot(j, ctx.ny) && is_hot(k, ctx.nz) ? 1.0
                                                                                           : 0.0);
}

// Per-step decay factor of fill_sine_mode's field.
inline double sine_mode_decay(std::size_t nx, std::size_t ny, std::size_t nz, double r) {
    using std::numbers::pi;
    auto sin_squared = [](double x) { return std::sin(x) * std::sin(x); };
    return 1.0 - 4.0 * r *
                     (sin_squared(pi / (2.0 * (nx + 1))) + sin_squared(pi / (2.0 * (ny + 1))) +
                      sin_squared(pi / (2.0 * (nz + 1))));
}

} // namespace scr

#endif // SCR_STENCIL_H
