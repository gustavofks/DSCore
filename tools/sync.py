#!/usr/bin/env python3
"""Everything to run when the DSi's SD card is in the PC, in one command.

Usage: python tools/sync.py <sd-root> [--no-backup] [--no-covers] [--no-metadata]

  1. backs up the saves (tools/backup_saves.py) before anything else touches the card,
  2. downloads box art for new games and rebuilds the grid thumbnails (tools/fetch_covers.py),
  3. rewrites genres, years and developers (tools/fetch_metadata.py).

Each step runs even if an earlier one failed; the summary at the end says how each went.
"""
import os
import subprocess
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
STEPS = [
    ("--no-backup", "Save backup", ["backup_saves.py"]),
    ("--no-covers", "Box art", ["fetch_covers.py"]),
    ("--no-metadata", "Genres and years", ["fetch_metadata.py"]),
]


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 1 or not os.path.isdir(args[0]):
        sys.exit(__doc__)
    sd_root = args[0]
    results = []
    for flag, title, script in STEPS:
        if flag in sys.argv:
            results.append((title, "skipped"))
            continue
        print(f"\n=== {title} ===", flush=True)
        code = subprocess.call([sys.executable, os.path.join(TOOLS, script[0]), sd_root])
        results.append((title, "ok" if code == 0 else f"failed (exit code {code})"))
    print("\n=== Summary ===")
    for title, result in results:
        print(f"  {title}: {result}")
    if any(result.startswith("failed") for _, result in results):
        sys.exit(1)


if __name__ == "__main__":
    main()
