## General stats for my office desktop

== STREAM ==
<comments>
Memory-bandwidth ceiling for the stencil. McCalpin stream.c 5.10, arrays of
200M doubles (1.6 GB each, >> 128 MB L3), 20 iterations; rates are STREAM's
best-of-20. STREAM counts 24 B/elt for Triad and excludes write-allocate; the
real DRAM traffic is 32 B/elt, so real Triad bandwidth ~= 31.6 * 4/3 ~= 42 GB/s.
Multi-threaded Triad doesn't scale past 1 thread: DRAM, not the core or the
CCD link, is the limit (8T on one CCD == 8T split across CCDs).
</comments>
date      2026-09-24 20:38 UTC
host      pop-os
cpu       AMD Ryzen 9 9950X3D 16-Core Processor, 32 logical
memory    249.3 GiB
governor  powersave
compiler  GNU 16.0.1, -O3 -march=native -fopenmp -mcmodel=medium
config                    Copy MB/s   Triad MB/s
1T cpu 2  (96 MB L3)        31653       31633
1T cpu 2  (repeat)          31059       31668
1T cpu 27 (32 MB L3)        31172       31342
8T cpus 0-7   (CCD0)        40873       30885
8T cpus 8-15  (CCD1)        41337       30891
8T cpus 0-3,8-11 (split)    41103       30608
16T one per core            39986       30247
32T all SMT                 26953       29590
triad     ~31.6 GB/s single thread (STREAM counting), ~42 GB/s real with write-allocate

## Stencil benchmarks run my office desktop

== stencil-lab bench ==

<comments>
This is the baseline.
</comments>
date      2026-09-24 20:44:07 UTC
host      pop-os
cpu       AMD Ryzen 9 9950X3D 16-Core Processor, 32 logical
ran on    cpu 27 -> cpu 27 (affinity: 32 cpus)
caches    L1d 48K  L2 1024K  L3 32768K (cpu 27)
memory    249.3 GiB
kernel    7.1.5-76070105-generic
governor  powersave
compiler  GNU 16.0.1
build     RelWithDebInfo [-O2 -g -DNDEBUG] commit 8bdd63a
solver    L0 naive
grid      512x512x512, 10 steps x 10 reps
time      median 1.5150 s   p99 1.5256 s
rate      7.09 GFLOP/s   21.26 GB/s (model)
checksum  34778955.772421293

== stencil-lab bench ==
<comments>
Here we used the same impl as the baseline but modified such that the access
pattern in the inner loop is not sequential.
</comments>
== stencil-lab bench ==
date      2026-09-24 21:31:58 UTC
host      pop-os
cpu       AMD Ryzen 9 9950X3D 16-Core Processor, 32 logical
ran on    cpu 1 -> cpu 27 (affinity: 32 cpus)
caches    L1d 48K  L2 1024K  L3 98304K (cpu 1)
memory    249.3 GiB
kernel    7.1.5-76070105-generic
governor  powersave
compiler  GNU 16.0.1
build     RelWithDebInfo [-O2 -g -DNDEBUG] commit 32523e9
solver    L0 naive - bad loop
grid      512x512x512, 10 steps x 10 reps
time      median 11.9996 s   p99 12.2030 s
rate      0.89 GFLOP/s   2.68 GB/s (model)
checksum  34778955.772421293
