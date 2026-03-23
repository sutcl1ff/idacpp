#!/usr/bin/env python3
"""Synchronize idalib_setup.cpp's runtime header list with ida_sdk.h.

Usage:
  python tools/sync_ida_sdk_headers.py [--check] [--sdk-dir /path/to/idasdk/src/include]

The curated source of truth is ida_sdk.h. This script:
  1. Parses all #include <...> lines from ida_sdk.h
  2. Rewrites g_idaHeaders[] in idalib_setup.cpp to match that exact order
  3. Optionally reports headers present in the SDK include directory that are
     not part of the current curated set
  4. Validates that no excluded header (EXCLUDED_HEADERS) appears in ida_sdk.h

It does not modify ida_sdk.h itself.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IDA_SDK_H = ROOT / "ida_sdk.h"
IDALIB_SETUP = ROOT / "idalib_setup.cpp"

# Headers that must NOT appear in ida_sdk.h or g_idaHeaders[].
EXCLUDED_HEADERS: set[str] = {
    # Pragma-only: globally suppresses deprecation warnings in the PCH.
    "allow_deprecated.hpp",
}


def parse_umbrella_headers(path: Path) -> list[str]:
    headers: list[str] = []
    pattern = re.compile(r'^\s*#include\s+<([^>]+)>\s*$')
    for line in path.read_text().splitlines():
        match = pattern.match(line)
        if match:
            headers.append(match.group(1))
    if not headers:
        raise RuntimeError(f"no headers parsed from {path}")
    return headers


def format_header_block(headers: list[str]) -> str:
    lines = ['static const char* g_idaHeaders[] = {']
    for index, header in enumerate(headers):
        suffix = "," if index + 1 < len(headers) else ""
        lines.append(f'    "{header}"{suffix}')
    lines.append("};")
    return "\n".join(lines)


def rewrite_runtime_list(headers: list[str], path: Path) -> bool:
    text = path.read_text()
    pattern = re.compile(
        r"static const char\* g_idaHeaders\[\] = \{\n.*?\n\};",
        re.DOTALL,
    )
    replacement = format_header_block(headers)
    updated, count = pattern.subn(replacement, text, count=1)
    if count != 1:
        raise RuntimeError(f"failed to locate g_idaHeaders block in {path}")
    if updated == text:
        return False
    path.write_text(updated)
    return True


def scan_sdk_dir(path: Path) -> list[str]:
    return sorted(p.name for p in path.iterdir() if p.is_file())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true",
                        help="report drift but do not rewrite files")
    parser.add_argument("--sdk-dir", type=Path,
                        help="optional IDA SDK include dir to scan for uncatalogued headers")
    args = parser.parse_args()

    headers = parse_umbrella_headers(IDA_SDK_H)

    # Verify no excluded headers leaked into ida_sdk.h
    leaked = EXCLUDED_HEADERS & set(headers)
    if leaked:
        for hdr in sorted(leaked):
            print(f"ERROR: excluded header '{hdr}' found in {IDA_SDK_H.name}", file=sys.stderr)
        if args.check:
            return 1
        # In sync mode, filter them out before writing
        headers = [h for h in headers if h not in EXCLUDED_HEADERS]

    current = IDALIB_SETUP.read_text()
    desired_block = format_header_block(headers)
    drifted = desired_block not in current

    if args.check:
        print("runtime header list:", "out of sync" if drifted else "in sync")
    else:
        changed = rewrite_runtime_list(headers, IDALIB_SETUP)
        print("runtime header list:", "updated" if changed else "already in sync")

    if args.sdk_dir:
        sdk_headers = scan_sdk_dir(args.sdk_dir)
        curated = set(headers)
        excluded_found = [h for h in sdk_headers if h in EXCLUDED_HEADERS]
        extras = [h for h in sdk_headers
                  if h not in curated and h not in EXCLUDED_HEADERS]
        print(f"SDK scan: {len(sdk_headers)} headers, "
              f"{len(extras)} not in ida_sdk.h, "
              f"{len(excluded_found)} excluded")
        for hdr in extras:
            print(f"  {hdr}")
        if excluded_found:
            print(f"  (excluded: {', '.join(excluded_found)})")

    return 1 if args.check and drifted else 0


if __name__ == "__main__":
    sys.exit(main())
