# Performance benchmark reports

The [report tool](../tests/perf_report.py) establishes the first part of roadmap 16-1:
it accepts raw repeated measurements, validates that the test environment and
workload are recorded, produces stable JSON summaries, and compares only
compatible runs. It adds no daemon, boot work, or resident kernel cost.

Run it with Python 3.10 or later:

```sh
python3 tests/perf_report.py summarize samples.json --output summary.json
python3 tests/perf_report.py compare baseline.json candidate.json
```

The input format is `suirabox-benchmark-v1`. Each run records the benchmark,
metric, unit, whether higher or lower values are better, machine and CPU
identity, CPU count, memory, QEMU version and machine type, compiler and flags,
build configuration/revision, workload parameters, and at least five raw
numeric samples. `summarize` reports minimum, median, nearest-rank p95, maximum,
and median absolute deviation. These statistics retain spread information;
they do not establish a performance claim by themselves.

`compare` permits the source revision to change, but requires identical machine
and VM metadata, workload, metric and units, compiler, flags, and build
configuration. It reports relative median change and applies the declared
direction to label improvement or regression. A zero baseline median cannot be
compared by percentage. The tool rejects missing metadata, non-finite values,
too few samples, or incompatible runs instead of silently combining them.

QEMU is useful for deterministic boot and correctness checks, but TCG timing is
not representative of native hardware performance. Record host identity and
QEMU configuration and compare only measurements from the same controlled
environment. No absolute FPS, latency, or speedup target is claimed here. The
next milestone is instrumented kernel scheduler and memory workloads with
explicit timer resolution and overhead accounting, followed by native-hardware
Minecraft/JVM and graphics workloads before any tuning claims.
