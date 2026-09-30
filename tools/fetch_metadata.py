#!/usr/bin/env python3
"""Writes genre, year, developer and player count of the games on a DSi SD card to metadata.ini.

Usage: python tools/fetch_metadata.py <sd-root>

  <sd-root>   SD card root (e.g. E:\\) or a copy of it; games are read from everywhere under roms/.

DS games are looked up by game code in GameTDB's DS database. The other consoles use libretro-database's
metadata files: GBA games by the No-Intro title of their game code, the rest by CRC32, with the file
name as the last resort. Genres are grouped into a short list (Action, Platform, RPG...) so the filter
on the DSi stays usable.

The result goes to _nds/DSCore/metadata.ini, one section per ROM file name (format in
arm9/source/core/Metadata.h). A section with "locked = 1" is kept as it is, so hand edits survive new
runs. Downloads are cached for a week. Only the Python standard library is used.
"""
import io
import os
import re
import sys
import tempfile
import time
import urllib.request
import xml.etree.ElementTree as ET
import zipfile

import fetch_covers as fc

GAMETDB_DS = "https://www.gametdb.com/dstdb.zip?LANG=EN&FALLBACK=TRUE"
LIBRETRO_META = "https://raw.githubusercontent.com/libretro/libretro-database/master/metadat/{kind}/{system}.dat"
CACHE_AGE = 7 * 24 * 3600
# DATS keys of fetch_covers.py -> libretro-database system name.
LIBRETRO_SYSTEMS = {
    "gba": "Nintendo - Game Boy Advance", "gb": "Nintendo - Game Boy", "gbc": "Nintendo - Game Boy Color",
    "nes": "Nintendo - Nintendo Entertainment System", "fds": "Nintendo - Family Computer Disk System",
    "snes": "Nintendo - Super Nintendo Entertainment System", "sms": "Sega - Master System - Mark III",
    "gg": "Sega - Game Gear", "md": "Sega - Mega Drive - Genesis", "a26": "Atari - 2600",
}
LIBRETRO_KINDS = {"genre": "genre", "releaseyear": "year", "developer": "developer", "maxusers": "players"}

# Genre groups, most specific first: a game listed as "role-playing, action" is an RPG.
GENRE_GROUPS = [
    ("RPG", ["role-playing", "rpg"]),
    ("Platform", ["platform"]),
    ("Fighting", ["fighting", "beat'em up", "beat 'em up", "brawler"]),
    ("Racing", ["racing", "driving"]),
    ("Shooter", ["shooter", "shoot'em up", "shoot 'em up", "lightgun"]),
    ("Puzzle", ["puzzle", "thinking", "quiz", "trivia", "brain"]),
    ("Sports", ["sport", "hunting", "fishing", "golf", "soccer", "tennis"]),
    ("Strategy", ["strategy", "tactic"]),
    ("Simulation", ["simulation", "pet", "life"]),
    ("Music", ["music", "rhythm", "dancing"]),
    ("Party & Casual", ["party", "board", "card", "gambling", "casino", "pinball", "casual", "mini-game"]),
    ("Adventure", ["adventure"]),
    ("Action", ["action", "arcade"]),
    ("Compilation", ["compilation"]),
]


def group_genre(text):
    """The group of a genre list such as "action,platformer" or "Role-playing (RPG)"; None if empty."""
    lowered = text.lower()
    if not lowered.strip():
        return None
    for group, words in GENRE_GROUPS:
        if any(word in lowered for word in words):
            return group
    return "Other"


def cached_download(url, name, headers=None):
    path = os.path.join(tempfile.gettempdir(), name)
    if not os.path.exists(path) or time.time() - os.path.getmtime(path) > CACHE_AGE:
        request = urllib.request.Request(url, headers=headers or {"User-Agent": "DSCore metadata fetcher"})
        try:
            with urllib.request.urlopen(request, timeout=60) as response:
                data = response.read()
            with open(path, "wb") as f:
                f.write(data)
        except Exception as error:
            print(f"Could not download {url}: {error}")
    if not os.path.exists(path):
        return None
    with open(path, "rb") as f:
        return f.read()


def load_gametdb_ds():
    """Game code -> metadata dict from GameTDB (it only answers browser-like user agents)."""
    data = cached_download(GAMETDB_DS, "dscore-dstdb.zip", {"User-Agent": "Mozilla/5.0 (compatible; DSCore)"})
    if not data or not data.startswith(b"PK"):
        print("GameTDB's DS database is unavailable; DS games get no metadata.")
        return {}
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        root = ET.fromstring(z.read(z.namelist()[0]))
    games = {}
    for game in root.iter("game"):
        code = (game.findtext("id") or "").strip()
        if len(code) != 4:
            continue
        meta = {}
        genre = group_genre(game.findtext("genre") or "")
        if genre:
            meta["genre"] = genre
        date = game.find("date")
        if date is not None and (date.get("year") or "").isdigit() and date.get("year") != "0":
            meta["year"] = date.get("year")
        developer = (game.findtext("developer") or "").strip()
        if developer and developer.lower() != "unknown":
            meta["developer"] = developer
        players = game.find("input")
        if players is not None and (players.get("players") or "").isdigit() and players.get("players") != "0":
            meta["players"] = players.get("players")
        games[code] = meta
    # The first three characters name the game in every region; fill what one region lacks from the
    # others (GameTDB often has the European entry complete and the American one empty).
    by_game = {}
    # The year is the earliest release in any region; later dates are often re-releases.
    by_game = {}
    for code in sorted(games, key=lambda c: "EPJ".find(c[3]) % 4):
        merged = by_game.setdefault(code[:3], {})
        for key, value in games[code].items():
            if key == "year" and "year" in merged:
                merged["year"] = min(merged["year"], value)
            else:
                merged.setdefault(key, value)
    for code, meta in games.items():
        for key, value in by_game.get(code[:3], {}).items():
            if key == "year":
                meta["year"] = value
            else:
                meta.setdefault(key, value)
    return games


def load_libretro(system):
    """(by CRC32, by No-Intro title) -> metadata dict, merged from libretro's genre/year/developer files."""
    by_crc, by_name = {}, {}
    for kind, key in LIBRETRO_KINDS.items():
        name = LIBRETRO_SYSTEMS[system]
        url = LIBRETRO_META.format(kind=kind, system=urllib.request.quote(name))
        data = cached_download(url, f"dscore-{kind}-{system}.dat")
        if not data or not data.lstrip().startswith(b"clrmamepro"):
            continue
        for block in data.decode("utf-8", "replace").split("\ngame (")[1:]:
            title = re.search(r'^\s*(?:comment|name) "([^"]+)"', block, re.M)
            value = re.search(rf'^\s*{kind if kind != "maxusers" else "users"} "?([^"\n]+?)"?\s*$', block, re.M)
            if not value:
                continue
            value = value.group(1).strip()
            if key == "genre":
                value = group_genre(value)
            elif key in ("year", "players") and not value.isdigit():
                continue
            if not value:
                continue
            for crc in re.findall(r"\bcrc ([0-9A-Fa-f]{8})", block):
                by_crc.setdefault(crc.upper(), {})[key] = value
            if title:
                by_name.setdefault(title.group(1), {})[key] = value
    return by_crc, by_name


def read_ini(path):
    """Existing metadata.ini: section -> {key: value}."""
    sections, current = {}, None
    if not os.path.exists(path):
        return sections
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line.startswith("[") and line.endswith("]"):
                current = sections.setdefault(line[1:-1], {})
            elif current is not None and "=" in line and not line.startswith(";"):
                key, _, value = line.partition("=")
                current[key.strip()] = value.strip()
    return sections


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    sd_root = sys.argv[1]
    target = os.path.join(sd_root, "_nds", "DSCore", "metadata.ini")
    roms = list(fc.list_roms(sd_root))
    systems = sorted({system for _, system in roms})
    print(f"{len(roms)} games; consoles: {', '.join(systems)}")

    gametdb = load_gametdb_ds() if "nds" in systems else {}
    libretro = {s: load_libretro(s) for s in systems if s in LIBRETRO_SYSTEMS}
    gba_titles = fc.load_titles("gba") if "gba" in systems else {}

    existing = read_ini(target)
    result, found = {}, 0
    for path, system in roms:
        name = os.path.basename(path)
        if existing.get(name, {}).get("locked") == "1":
            result[name] = existing[name]
            continue
        meta = {}
        if system == "nds":
            code = fc.game_code(path, 0x0C)
            meta = dict(gametdb.get(code, {})) if code else {}
        elif system in libretro:
            by_crc, by_name = libretro[system]
            names = []
            if system == "gba":
                code = fc.game_code(path, 0xAC)
                names = gba_titles.get(code, []) if code else []
            names += fc.name_guesses(path)
            meta = next((dict(by_name[n]) for n in names if n in by_name), {})
            if not meta and (system != "gba" or os.path.getsize(path) <= 32 * 1024 * 1024):
                meta = next((dict(by_crc[c]) for c in fc.crc32s(path) if c in by_crc), {})
        if meta:
            found += 1
            result[name] = meta
        else:
            print(f"  no metadata: {name}")

    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target, "w", encoding="utf-8", newline="\n") as f:
        f.write("; Written by tools/fetch_metadata.py. Add \"locked = 1\" to a section to keep your edits.\n")
        for name in sorted(result):
            f.write(f"\n[{name}]\n")
            for key in ("genre", "year", "developer", "players", "locked"):
                if result[name].get(key):
                    f.write(f"{key} = {result[name][key]}\n")
    print(f"\nMetadata for {found} of {len(roms)} games written to {target}")


if __name__ == "__main__":
    main()
