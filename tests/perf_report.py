#!/usr/bin/env python3
"""Validate and compare repeatable SuiraBox performance sample sets."""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from pathlib import Path
from typing import Any


FORMAT = "suirabox-benchmark-v1"
ENVIRONMENT_FIELDS = ("machine", "cpu", "cpu_count", "memory_bytes", "qemu", "machine_type")
BUILD_FIELDS = ("revision", "compiler", "compiler_flags", "config")


class ReportError(ValueError):
    pass


def _object(value: Any, name: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ReportError(f"{name} must be an object")
    return value


def _nonempty_string(value: Any, name: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ReportError(f"{name} must be a non-empty string")
    return value


def load_samples(path: Path) -> dict[str, Any]:
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ReportError(f"cannot read JSON samples: {error}") from error
    root = _object(document, "document")
    if root.get("format") != FORMAT:
        raise ReportError(f"format must be {FORMAT!r}")
    for field in ("benchmark", "metric", "unit"):
        _nonempty_string(root.get(field), field)
    if root.get("direction") not in ("higher", "lower"):
        raise ReportError("direction must be 'higher' or 'lower'")

    environment = _object(root.get("environment"), "environment")
    for field in ENVIRONMENT_FIELDS:
        value = environment.get(field)
        if field in ("cpu_count", "memory_bytes"):
            if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
                raise ReportError(f"environment.{field} must be a positive integer")
        else:
            _nonempty_string(value, f"environment.{field}")

    build = _object(root.get("build"), "build")
    for field in BUILD_FIELDS:
        _nonempty_string(build.get(field), f"build.{field}")
    workload = _object(root.get("workload"), "workload")
    _nonempty_string(workload.get("name"), "workload.name")
    _object(workload.get("parameters"), "workload.parameters")

    samples = root.get("samples")
    if not isinstance(samples, list) or len(samples) < 5:
        raise ReportError("samples must contain at least five repeated measurements")
    values: list[float] = []
    for index, sample in enumerate(samples):
        item = _object(sample, f"samples[{index}]")
        value = item.get("value")
        if not isinstance(value, (int, float)) or isinstance(value, bool):
            raise ReportError(f"samples[{index}].value must be a finite number")
        try:
            numeric = float(value)
        except (OverflowError, ValueError):
            raise ReportError(f"samples[{index}].value must be a finite number") from None
        if not math.isfinite(numeric):
            raise ReportError(f"samples[{index}].value must be a finite number")
        values.append(numeric)
    root["_values"] = values
    return root


def summarize(document: dict[str, Any]) -> dict[str, Any]:
    values = document["_values"]
    ordered = sorted(values)
    p95 = ordered[max(0, math.ceil(0.95 * len(ordered)) - 1)]
    median = statistics.median(values)
    mad = statistics.median(abs(value - median) for value in values)
    return {
        "format": FORMAT,
        "benchmark": document["benchmark"],
        "metric": document["metric"],
        "unit": document["unit"],
        "direction": document["direction"],
        "environment": document["environment"],
        "build": document["build"],
        "workload": document["workload"],
        "sample_count": len(values),
        "minimum": min(values),
        "median": median,
        "p95_nearest_rank": p95,
        "maximum": max(values),
        "median_absolute_deviation": mad,
    }


def compare(baseline: dict[str, Any], candidate: dict[str, Any]) -> dict[str, Any]:
    # Revision may differ by design. Compiler/config, hardware/VM and workload
    # must match so a comparison cannot quietly mix different test conditions.
    for field in ("benchmark", "metric", "unit", "direction", "environment", "workload"):
        if baseline[field] != candidate[field]:
            raise ReportError(f"comparison requires matching {field}")
    for field in ("compiler", "compiler_flags", "config"):
        if baseline["build"][field] != candidate["build"][field]:
            raise ReportError(f"comparison requires matching build.{field}")

    before = summarize(baseline)["median"]
    after = summarize(candidate)["median"]
    if before == 0:
        raise ReportError("baseline median must be non-zero for a relative comparison")
    raw_change = (after - before) / abs(before) * 100.0
    improvement = raw_change if baseline["direction"] == "higher" else -raw_change
    return {
        "format": FORMAT,
        "benchmark": baseline["benchmark"],
        "metric": baseline["metric"],
        "unit": baseline["unit"],
        "baseline_revision": baseline["build"]["revision"],
        "candidate_revision": candidate["build"]["revision"],
        "baseline_median": before,
        "candidate_median": after,
        "raw_change_percent": raw_change,
        "improvement_percent": improvement,
        "direction": baseline["direction"],
        "interpretation": "candidate faster/better" if improvement > 0 else "candidate slower/worse" if improvement < 0 else "no median change",
    }


def write_json(value: dict[str, Any], output: Path | None) -> None:
    rendered = json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n"
    if output is None:
        sys.stdout.write(rendered)
    else:
        output.write_text(rendered, encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    summarize_parser = commands.add_parser("summarize", help="summarize repeated raw measurements")
    summarize_parser.add_argument("samples", type=Path)
    summarize_parser.add_argument("--output", type=Path)
    compare_parser = commands.add_parser("compare", help="compare compatible baseline and candidate sample sets")
    compare_parser.add_argument("baseline", type=Path)
    compare_parser.add_argument("candidate", type=Path)
    compare_parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == "summarize":
            write_json(summarize(load_samples(args.samples)), args.output)
        else:
            write_json(compare(load_samples(args.baseline), load_samples(args.candidate)), args.output)
    except (ReportError, OSError) as error:
        print(f"benchmark report error: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
