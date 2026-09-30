#!/usr/bin/env python3
"""Backs up the game saves on a DSi SD card to the PC, and restores them.

Usage:
  python tools/backup_saves.py <sd-root> [--dest <folder>]
  python tools/backup_saves.py --restore <backup-folder> <sd-root> [--apply]

Backup copies every save under roms/ (.sav, .srm, .sra, .dsv, .mcr, including nds-bootstrap's saves/
folders), the emulator data under data/ (e.g. S8DS) and DSCore's favorites, history and settings into
<dest>/<date>_<time>/, keeping the SD card's folder layout. <dest> defaults to "DSCore saves" in your
home folder. When nothing changed since the last backup, no new folder is made.

Restore lists what it would copy back; add --apply to copy. A save on the SD card that differs from the
backup is kept next to it as <name>.before-restore first.
Only the Python standard library is used.
"""
import datetime
import hashlib
import os
import shutil
import sys

SAVE_EXTENSIONS = {".sav", ".srm", ".sra", ".dsv", ".mcr"}
DSCORE_FILES = ["_nds/DSCore/userdata.ini", "_nds/DSCore/config.ini"]
MAX_DATA_FILE = 4 * 1024 * 1024  # skip large files under data/, which are not saves
MANIFEST = "manifest.txt"


def long_path(path):
    """On Windows, lets file calls go past the 260-character path limit (deep ROM folders)."""
    path = os.path.abspath(path)
    if os.name == "nt" and not path.startswith("\\\\?\\"):
        return "\\\\?\\" + path
    return path


def copy(src, dst):
    os.makedirs(long_path(os.path.dirname(dst)), exist_ok=True)
    shutil.copy2(long_path(src), long_path(dst))


def digest(path):
    h = hashlib.sha1()
    with open(long_path(path), "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def find_saves(sd_root):
    """Paths relative to sd_root, with forward slashes, of every file to back up."""
    found = []
    for root, _, files in os.walk(os.path.join(sd_root, "roms")):
        for name in files:
            if os.path.splitext(name)[1].lower() in SAVE_EXTENSIONS and not name.startswith("."):
                found.append(os.path.join(root, name))
    for root, _, files in os.walk(os.path.join(sd_root, "data")):
        for name in files:
            path = os.path.join(root, name)
            if os.path.getsize(path) <= MAX_DATA_FILE:
                found.append(path)
    for rel in DSCORE_FILES:
        path = os.path.join(sd_root, rel)
        if os.path.isfile(path):
            found.append(path)
    return sorted(os.path.relpath(p, sd_root).replace("\\", "/") for p in found)


def read_manifest(folder):
    entries = {}
    path = os.path.join(folder, MANIFEST)
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            for line in f:
                sha, _, rel = line.rstrip("\n").partition("  ")
                if rel:
                    entries[rel] = sha
    return entries


def latest_backup(dest):
    if not os.path.isdir(dest):
        return None
    folders = sorted(d for d in os.listdir(dest) if os.path.exists(os.path.join(dest, d, MANIFEST)))
    return os.path.join(dest, folders[-1]) if folders else None


def backup(sd_root, dest):
    saves = find_saves(sd_root)
    if not saves:
        sys.exit(f"No saves found under {sd_root}")
    current = {rel: digest(os.path.join(sd_root, rel)) for rel in saves}
    previous = latest_backup(dest)
    if previous and read_manifest(previous) == current:
        print(f"Nothing changed since {previous} ({len(current)} files).")
        return
    folder = os.path.join(dest, datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S"))
    for rel in saves:
        copy(os.path.join(sd_root, rel), os.path.join(folder, rel))
    with open(os.path.join(folder, MANIFEST), "w", encoding="utf-8", newline="\n") as f:
        for rel in saves:
            f.write(f"{current[rel]}  {rel}\n")
    old = read_manifest(previous) if previous else {}
    changed = [rel for rel in saves if old.get(rel) != current[rel]]
    print(f"Backed up {len(saves)} files to {folder}")
    if previous:
        print(f"{len(changed)} new or changed since the last backup:")
        for rel in changed:
            print(f"  {rel}")


def restore(folder, sd_root, apply):
    manifest = read_manifest(folder)
    if not manifest:
        sys.exit(f"{folder} is not a backup made by this script (no {MANIFEST}).")
    plan = []
    for rel, sha in sorted(manifest.items()):
        target = os.path.join(sd_root, rel)
        exists = os.path.exists(long_path(target))
        if exists and digest(target) == sha:
            continue
        plan.append((rel, exists))
    if not plan:
        print("The SD card already has every save of this backup.")
        return
    for rel, exists in plan:
        print(f"  {'replace' if exists else 'add    '} {rel}")
    if not apply:
        print(f"\n{len(plan)} files would be copied. Run again with --apply to copy them.")
        return
    for rel, exists in plan:
        target = os.path.join(sd_root, rel)
        if exists:
            copy(target, target + ".before-restore")
        copy(os.path.join(folder, rel), target)
    print(f"\nRestored {len(plan)} files.")


def main():
    args = sys.argv[1:]
    if "--restore" in args:
        rest = [a for a in args if a not in ("--restore", "--apply")]
        if len(rest) != 2:
            sys.exit(__doc__)
        restore(rest[0], rest[1], "--apply" in args)
        return
    dest = os.path.join(os.path.expanduser("~"), "DSCore saves")
    if "--dest" in args:
        i = args.index("--dest")
        if i + 1 >= len(args):
            sys.exit(__doc__)
        dest = args[i + 1]
        args = args[:i] + args[i + 2:]
    if len(args) != 1:
        sys.exit(__doc__)
    backup(args[0], dest)


if __name__ == "__main__":
    main()
