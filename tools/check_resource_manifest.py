#!/usr/bin/env python3
"""Validate the built-in Resource manifest without loading asset payloads."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "resources" / "core-manifest.json"
VALID_TIERS = {"builtin", "optional", "remote"}
REQUIRED_CORE_IDS = {
    "core/ui",
    "core/font/default",
    "core/theme/fallback",
    "core/wallpaper/default",
    "core/input",
    "core/recovery",
    "core/settings",
    "core/terminal",
    "core/file-manager-basic",
}


def main() -> int:
    try:
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"error: cannot read resource manifest: {exc}")
        return 1

    if manifest.get("schema") != 1 or manifest.get("policy") != "core-optional-remote":
        print("error: unsupported manifest schema/policy")
        return 1
    if manifest.get("offline_baseline") is not True:
        print("error: offline baseline must be true")
        return 1

    resources = manifest.get("resources")
    if not isinstance(resources, list) or not resources:
        print("error: resource list is empty")
        return 1

    seen: set[str] = set()
    builtin_required: set[str] = set()
    for entry in resources:
        if not isinstance(entry, dict):
            print("error: resource entry is not an object")
            return 1
        resource_id = entry.get("id")
        tier = entry.get("tier")
        required = entry.get("required")
        mutable = entry.get("mutable")
        if not isinstance(resource_id, str) or not resource_id or resource_id in seen:
            print(f"error: invalid or duplicate resource id: {resource_id!r}")
            return 1
        seen.add(resource_id)
        if tier not in VALID_TIERS:
            print(f"error: invalid resource tier for {resource_id}")
            return 1
        if not isinstance(required, bool) or not isinstance(mutable, bool):
            print(f"error: required/mutable must be booleans for {resource_id}")
            return 1
        if tier == "builtin":
            if mutable:
                print(f"error: builtin resource cannot be mutable: {resource_id}")
                return 1
            if required:
                builtin_required.add(resource_id)
        elif required:
            print(f"error: optional/remote resource cannot be required: {resource_id}")
            return 1

    missing = REQUIRED_CORE_IDS - builtin_required
    if missing:
        print("error: required Core resources missing:")
        for resource_id in sorted(missing):
            print(f"  - {resource_id}")
        return 1

    print(f"resource manifest policy: OK ({len(resources)} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
