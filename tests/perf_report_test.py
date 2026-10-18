import json
import tempfile
import unittest
from pathlib import Path

from perf_report import ReportError, compare, load_samples, summarize


def document(values=(1, 2, 3, 4, 5), *, revision="a", direction="lower"):
    return {
        "format": "suirabox-benchmark-v1",
        "benchmark": "scheduler-wakeup",
        "metric": "latency",
        "unit": "pit_ticks",
        "direction": direction,
        "environment": {
            "machine": "ci-runner",
            "cpu": "QEMU TCG",
            "cpu_count": 2,
            "memory_bytes": 536870912,
            "qemu": "9.2.0",
            "machine_type": "q35",
        },
        "build": {
            "revision": revision,
            "compiler": "gcc 14",
            "compiler_flags": "-O2 -m64",
            "config": "default",
        },
        "workload": {"name": "sleep-wake", "parameters": {"iterations": 10}},
        "samples": [{"value": value} for value in values],
    }


class PerfReportTests(unittest.TestCase):
    def test_summary_uses_median_nearest_rank_p95_and_mad(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "samples.json"
            path.write_text(json.dumps(document((1, 2, 3, 4, 100))), encoding="utf-8")
            summary = summarize(load_samples(path))
        self.assertEqual(summary["median"], 3)
        self.assertEqual(summary["p95_nearest_rank"], 100)
        self.assertEqual(summary["median_absolute_deviation"], 1)
        self.assertEqual(summary["sample_count"], 5)

    def test_compare_accounts_for_lower_and_higher_is_better(self):
        before = load_samples_from(document((10, 10, 10, 10, 10), revision="old"))
        faster = load_samples_from(document((8, 8, 8, 8, 8), revision="new"))
        self.assertEqual(compare(before, faster)["improvement_percent"], 20)
        throughput = load_samples_from(document((8, 8, 8, 8, 8), revision="new", direction="higher"))
        with self.assertRaisesRegex(ReportError, "direction"):
            compare(before, throughput)

    def test_compare_rejects_different_machine_workload_or_compiler(self):
        baseline = load_samples_from(document())
        for change, expected in (
            (lambda d: d["environment"].update(cpu="different CPU"), "environment"),
            (lambda d: d["workload"]["parameters"].update(iterations=11), "workload"),
            (lambda d: d["build"].update(compiler="gcc 15"), "build.compiler"),
        ):
            candidate = document()
            change(candidate)
            with self.subTest(expected=expected), self.assertRaisesRegex(ReportError, expected):
                compare(baseline, load_samples_from(candidate))

    def test_rejects_short_nonfinite_and_malformed_sample_sets(self):
        invalid = [document((1, 2, 3, 4)), document((1, 2, 3, 4, float("nan")))]
        for raw in invalid:
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "samples.json"
                path.write_text(json.dumps(raw), encoding="utf-8")
                with self.assertRaises(ReportError):
                    load_samples(path)


def load_samples_from(value):
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "samples.json"
        path.write_text(json.dumps(value), encoding="utf-8")
        return load_samples(path)


if __name__ == "__main__":
    unittest.main()
