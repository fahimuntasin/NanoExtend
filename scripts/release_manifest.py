#!/usr/bin/env python3
"""Create reproducible NanoExtend release metadata."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def describe(path: Path, base_url: str) -> dict[str, object]:
    data = path.read_bytes()
    return {
        "name": path.name,
        "file": f"{base_url.rstrip('/')}/{path.name}",
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--factory", type=Path, required=True)
    parser.add_argument("--ota", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--base-url", required=True)
    args = parser.parse_args()

    manifest = {
        "schema": 1,
        "project": "NanoExtend",
        "version": args.version,
        "channel": "stable",
        "hardware": "esp32dev-4mb",
        "releaseNotesUrl": (
            f"https://github.com/fahimuntasin/NanoExtend/releases/tag/v{args.version}"
        ),
        "factory": describe(args.factory, args.base_url),
        "ota": describe(args.ota, args.base_url),
    }
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
