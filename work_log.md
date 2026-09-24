### 2026-09-24

>[NOTE] standard benchmark grid size is `512x512x512`

- Started with a baseline impl that runs on a single cpu, got the following:
  `rate   7.09 GLOP/s   21.26 GB/s`
  The stats are logged in the benchmark log.
  The baseline impl already iterates in a way that memory-adjacent values are accessed
  in the inner loop, so we benefit from the mechanics of how data is fetched.
- I want to try silly things to see what makes it worse
  - poor loop order:
    `rate      0.85 GFLOP/s   2.54 GB/s (model)`
    We attribute this degraded performance to the access pattern. Data are stored
    such that values on the x-axis are adjacent in memory. When we iterate along the
    z-axis in the inner loop we make large jumps in memory and forego benefits of
    accessing data that gets pulled in on the same cache line.
