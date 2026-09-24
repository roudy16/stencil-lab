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
