#!/usr/bin/env python3
"""Validate benchmark evidence without overstating historical provenance.

Historical reports are accepted as historical evidence, but are never classified
as controlled-release measurements unless the required provenance is present.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPORT = ROOT / "binary-skill" / "benchmark_results.json"
BINARY = ROOT / "binary-skill" / "secondbrain.bin"
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def main() -> int:
    try:
        report = json.loads(REPORT.read_text(encoding="utf-8"))
        require(isinstance(report, dict), "report root must be an object")
        require(report.get("schema_version") == 1, "unsupported schema_version")
        provenance = report.get("provenance")
        require(isinstance(provenance, dict), "provenance must be an object")
        for field in ("os", "compiler", "build_type", "iterations_per_op"):
            require(field in provenance, f"missing provenance field: {field}")
        require(isinstance(provenance["iterations_per_op"], int)
                and provenance["iterations_per_op"] > 0,
                "iterations_per_op must be a positive integer")

        data = BINARY.read_bytes()
        require(report.get("file_size_bytes") == len(data),
                "binary size does not match report")
        expected_hash = report.get("file_sha256")
        require(isinstance(expected_hash, str) and SHA256_RE.fullmatch(expected_hash),
                "file_sha256 must be a lowercase SHA-256 hex digest")
        actual_hash = hashlib.sha256(data).hexdigest()
        require(actual_hash == expected_hash, "binary SHA-256 does not match report")
        require(isinstance(report.get("skill_count"), int) and report["skill_count"] > 0,
                "skill_count must be a positive integer")

        results = report.get("results")
        require(isinstance(results, list) and results, "results must be a non-empty list")
        seen = set()
        for index, item in enumerate(results):
            require(isinstance(item, dict), f"results[{index}] must be an object")
            operation = item.get("operation")
            require(isinstance(operation, str) and operation,
                    f"results[{index}].operation must be a non-empty string")
            require(operation not in seen, f"duplicate operation: {operation}")
            seen.add(operation)
            for field in ("samples", "min_ns", "median_ns", "p95_ns", "max_ns", "matches"):
                require(isinstance(item.get(field), int) and item[field] >= 0,
                        f"results[{index}].{field} must be a non-negative integer")
            require(item["samples"] > 0, f"results[{index}].samples must be positive")
            require(item["min_ns"] <= item["median_ns"] <= item["p95_ns"] <= item["max_ns"],
                    f"invalid timing order for {operation}")

        # Do not treat a legacy abbreviated commit + Release label as controlled
        # provenance. Controlled release requires all fields below.
        controlled_fields = ("commit", "source_dirty", "bench_context", "build_type")
        controlled = (
            provenance.get("build_type") == "Release"
            and isinstance(provenance.get("commit"), str)
            and bool(re.fullmatch(r"[0-9a-f]{40}", provenance["commit"]))
            and provenance.get("source_dirty") is False
            and provenance.get("bench_context") == "controlled-release"
        )
        if not all(field in provenance for field in controlled_fields) or not controlled:
            print("NOTICE: valid historical/smoke evidence; NOT certified as a controlled release.")
        print(f"PASS: {len(results)} benchmark operations; binary bytes={len(data)}; sha256={actual_hash}")
        return 0
    except (OSError, json.JSONDecodeError, ValueError, TypeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
