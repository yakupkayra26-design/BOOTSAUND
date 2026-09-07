#!/usr/bin/env python3
"""Validate metadata for authorized Turkish patch downloads."""

import json
import sys
from pathlib import Path

REQUIRED = {"id", "title_id", "version", "title", "url", "sha256", "license"}


def main(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if data.get("format") != 1 or not isinstance(data.get("patches"), list):
        raise ValueError("catalog must use format 1 and contain patches[]")
    seen = set()
    for patch in data["patches"]:
        missing = REQUIRED - set(patch)
        if missing:
            raise ValueError(f"missing fields: {sorted(missing)}")
        if patch["id"] in seen:
            raise ValueError(f"duplicate patch id: {patch['id']}")
        seen.add(patch["id"])
        digest = patch["sha256"].lower()
        if len(digest) != 64 or any(character not in "0123456789abcdef" for character in digest):
            raise ValueError(f"invalid sha256: {patch['id']}")
        if not patch["url"].startswith("https://"):
            raise ValueError(f"patch URL must use HTTPS: {patch['id']}")
    print(f"valid catalog: {len(data['patches'])} patch entries")


if __name__ == "__main__":
    try:
        main(sys.argv[1] if len(sys.argv) > 1 else "patches/catalog.json")
    except (IndexError, OSError, ValueError, json.JSONDecodeError) as error:
        print(f"invalid catalog: {error}", file=sys.stderr)
        raise SystemExit(1)