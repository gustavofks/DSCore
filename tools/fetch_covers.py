#!/usr/bin/env python3
"""Downloads box art for the games on a DSi SD card and converts it for DSCore.

Usage: python tools/fetch_covers.py <sd-root> [--twilight] [--force]

  <sd-root>   SD card root (e.g. E:\\) or a copy of it; games are read from everywhere under roms/.
  --twilight  Also save the original PNGs where TWiLight Menu++ looks for box art.
  --force     Download again covers that already exist.

DS covers come from GameTDB by game code. When GameTDB has none, and for GBA games, the game code read
from the ROM is looked up in libretro-database's No-Intro data to get the exact title that
libretro-thumbnails uses. Games of the other consoles have no game code: their CRC32 is looked up
instead. The file name is the last resort.
Each cover is scaled to fit 112x112 and written to _nds/DSCore/covers/<rom file name>.bin in DSCore's
format: "DSCV", width and height (u16 LE), then width*height DS colors (u16 LE, bit 15 set).
Every run also rebuilds _nds/DSCore/thumbs.bin, 40x40 thumbnails of all covers for the grid (format in
arm9/source/core/Thumbs.h).
Only the Python standard library is used.
"""
import os
import re
import struct
import sys
import tempfile
import time
import urllib.parse
import urllib.request
import zlib

MAX_SIZE = 112
THUMB_SIZE = 40  # grid thumbnails, core/Thumbs.h kThumbMaxSize
GAMETDB = "https://art.gametdb.com/ds/coverS/{region}/{code}.png"
LIBRETRO_THUMBS = "https://raw.githubusercontent.com/libretro-thumbnails/{system}/master/Named_Boxarts/{name}.png"
LIBRETRO_DB = "https://raw.githubusercontent.com/libretro/libretro-database/master/metadat/"
DATS = {  # system -> (thumbnail repository, No-Intro database file)
    "nds": ("Nintendo_-_Nintendo_DS", "no-intro/Nintendo%20-%20Nintendo%20DS.dat"),
    "gba": ("Nintendo_-_Game_Boy_Advance", "no-intro/Nintendo%20-%20Game%20Boy%20Advance.dat"),
    "gb": ("Nintendo_-_Game_Boy", "no-intro/Nintendo%20-%20Game%20Boy.dat"),
    "gbc": ("Nintendo_-_Game_Boy_Color", "no-intro/Nintendo%20-%20Game%20Boy%20Color.dat"),
    "nes": ("Nintendo_-_Nintendo_Entertainment_System", "no-intro/Nintendo%20-%20Nintendo%20Entertainment%20System.dat"),
    "fds": ("Nintendo_-_Family_Computer_Disk_System", "no-intro/Nintendo%20-%20Family%20Computer%20Disk%20System.dat"),
    "sms": ("Sega_-_Master_System_-_Mark_III", "no-intro/Sega%20-%20Master%20System%20-%20Mark%20III.dat"),
    "gg": ("Sega_-_Game_Gear", "no-intro/Sega%20-%20Game%20Gear.dat"),
    "snes": ("Nintendo_-_Super_Nintendo_Entertainment_System",
             "no-intro/Nintendo%20-%20Super%20Nintendo%20Entertainment%20System.dat"),
    "a26": ("Atari_-_2600", "no-intro/Atari%20-%202600.dat"),
}
# The extensions DSCore recognizes (core/Systems.cpp), mapped to DATS keys.
EXTENSIONS = {".nds": "nds", ".gba": "gba", ".gb": "gb", ".sgb": "gb", ".gbc": "gbc", ".nes": "nes", ".fds": "fds",
              ".sms": "sms", ".gg": "gg", ".sfc": "snes", ".smc": "snes", ".a26": "a26"}
DAT_MAX_AGE = 7 * 24 * 3600
REGION_PREFERENCE = ["(USA", "(World", "(Europe"]
# GameTDB region folder from the last letter of a DS game code.
REGIONS = {"E": ["US"], "P": ["EN", "US"], "J": ["JA"], "K": ["KO"], "F": ["FR", "EN"], "D": ["DE", "EN"],
           "S": ["ES", "EN"], "I": ["IT", "EN"], "H": ["NL", "EN"], "U": ["AU", "EN"], "O": ["US", "EN"]}
REGION_SUFFIXES = ["", " (USA)", " (USA, Europe)", " (Europe)", " (World)", " (Japan)"]


# --- PNG decoding (8-bit, non-interlaced; gray, RGB, palette, gray+alpha, RGBA) -------------------

def decode_png(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos, idat, palette = 8, b"", None
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        chunk = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
        elif kind == b"PLTE":
            palette = [tuple(chunk[i:i + 3]) for i in range(0, len(chunk), 3)]
        elif kind == b"IDAT":
            idat += chunk
        elif kind == b"IEND":
            break
    if depth != 8 or interlace != 0 or color not in (0, 2, 3, 4, 6):
        raise ValueError(f"unsupported PNG (depth {depth}, color {color}, interlace {interlace})")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color]
    raw = zlib.decompress(idat)
    stride = width * channels
    rows, prev = [], bytearray(stride)
    for y in range(height):
        base = y * (stride + 1)
        kind, line = raw[base], bytearray(raw[base + 1:base + 1 + stride])
        for i in range(stride):
            left = line[i - channels] if i >= channels else 0
            up = prev[i]
            corner = prev[i - channels] if i >= channels else 0
            if kind == 1:
                line[i] = (line[i] + left) & 255
            elif kind == 2:
                line[i] = (line[i] + up) & 255
            elif kind == 3:
                line[i] = (line[i] + (left + up) // 2) & 255
            elif kind == 4:
                p = left + up - corner
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - corner)
                line[i] = (line[i] + (left if pa <= pb and pa <= pc else up if pb <= pc else corner)) & 255
        rows.append(line)
        prev = line
    pixels = []
    for line in rows:
        for x in range(width):
            px = line[x * channels:(x + 1) * channels]
            if color == 0:
                pixels.append((px[0], px[0], px[0]))
            elif color == 2:
                pixels.append(tuple(px))
            elif color == 3:
                pixels.append(palette[px[0]])
            elif color == 4:
                pixels.append((px[0], px[0], px[0]))
            else:
                pixels.append(tuple(px[:3]))
    return width, height, pixels


# --- Scaling and DSCore format --------------------------------------------------------------------

def fit(width, height, limit=MAX_SIZE):
    scale = min(limit / width, limit / height, 1.0)
    return max(1, round(width * scale)), max(1, round(height * scale))


def scale_box(width, height, pixels, out_w, out_h):
    """Area-average downscale: each output pixel averages the source pixels it covers."""
    out = []
    for oy in range(out_h):
        y0, y1 = oy * height // out_h, max(oy * height // out_h + 1, (oy + 1) * height // out_h)
        for ox in range(out_w):
            x0, x1 = ox * width // out_w, max(ox * width // out_w + 1, (ox + 1) * width // out_w)
            r = g = b = n = 0
            for y in range(y0, y1):
                row = y * width
                for x in range(x0, x1):
                    pr, pg, pb = pixels[row + x]
                    r, g, b, n = r + pr, g + pg, b + pb, n + 1
            out.append((r // n, g // n, b // n))
    return out


def to_dscore(png):
    width, height, pixels = decode_png(png)
    out_w, out_h = fit(width, height)
    scaled = scale_box(width, height, pixels, out_w, out_h)
    body = b"".join(struct.pack("<H", 0x8000 | (b >> 3) << 10 | (g >> 3) << 5 | (r >> 3)) for r, g, b in scaled)
    return b"DSCV" + struct.pack("<HH", out_w, out_h) + body


# --- Grid thumbnails (thumbs.bin, read by core/Thumbs.h) -------------------------------------------

def read_dscore_cover(path):
    """(width, height, [(r, g, b), ...]) from a DSCV file, 8 bits per channel."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"DSCV" or len(data) < 8:
        return None
    width, height = struct.unpack_from("<HH", data, 4)
    if len(data) < 8 + width * height * 2:
        return None
    pixels = []
    for (c,) in struct.iter_unpack("<H", data[8:8 + width * height * 2]):
        pixels.append(((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3))
    return width, height, pixels


def fnv1a(text):
    value = 2166136261
    for byte in text.encode("utf-8"):
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def write_thumbs(covers_dir, rom_names, target):
    """Writes thumbs.bin with a THUMB_SIZE thumbnail of every game that has a cover."""
    thumbs = {}
    for name in rom_names:
        cover = os.path.join(covers_dir, name + ".bin")
        decoded = read_dscore_cover(cover) if os.path.exists(cover) else None
        if not decoded:
            continue
        width, height, pixels = decoded
        out_w, out_h = fit(width, height, THUMB_SIZE)
        scaled = scale_box(width, height, pixels, out_w, out_h)
        body = b"".join(struct.pack("<H", 0x8000 | (b >> 3) << 10 | (g >> 3) << 5 | (r >> 3)) for r, g, b in scaled)
        thumbs[fnv1a(name)] = (out_w, out_h, body)
    keys = sorted(thumbs)
    offset = 12 + 12 * len(keys)
    index, bodies = [], []
    for key in keys:
        width, height, body = thumbs[key]
        index.append(struct.pack("<IIBBH", key, offset, width, height, 0))
        bodies.append(body)
        offset += len(body)
    with open(target, "wb") as f:
        f.write(b"DSTH" + struct.pack("<HHI", 1, 0, len(keys)) + b"".join(index) + b"".join(bodies))
    return len(keys)


# --- Sources --------------------------------------------------------------------------------------

def download(url):
    request = urllib.request.Request(url, headers={"User-Agent": "DSCore cover fetcher"})
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            return response.read()
    except Exception:
        return None


def game_code(path, offset):
    with open(path, "rb") as f:
        f.seek(offset)
        code = f.read(4).decode("ascii", "replace")
    return code if code.isalnum() and len(code) == 4 else None


def load_dat(system):
    """The system's No-Intro DAT from libretro-database, cached for a week; empty when unavailable."""
    cache = os.path.join(tempfile.gettempdir(), f"dscore-{system}.dat")
    if not os.path.exists(cache) or time.time() - os.path.getmtime(cache) > DAT_MAX_AGE:
        data = download(LIBRETRO_DB + DATS[system][1])
        if data:
            with open(cache, "wb") as f:
                f.write(data)
    if not os.path.exists(cache):
        return ""
    return open(cache, encoding="utf-8", errors="replace").read()


def rank(title):
    return next((i for i, region in enumerate(REGION_PREFERENCE) if region in title), len(REGION_PREFERENCE))


def load_titles(system):
    """Key -> No-Intro titles (best region first): game codes for DS and GBA, CRC32s otherwise."""
    titles = {}
    for block in load_dat(system).split("\ngame (")[1:]:
        name = re.search(r'^\s*(?:name|comment) "([^"]+)"', block, re.M)
        if not name:
            continue
        if system in ("nds", "gba"):
            keys = re.findall(r'^\s*serial "(?:AGB-)?([0-9A-Z]{4})', block, re.M)
        else:
            keys = re.findall(r'\bcrc ([0-9A-Fa-f]{8})', block)
        for key in keys:
            titles.setdefault(key.upper(), []).append(name.group(1))
    return {key: sorted(names, key=rank) for key, names in titles.items()}


def fetch_thumbnail(system, names):
    for name in dict.fromkeys(names):  # unique, in order
        # libretro-thumbnails replaces these characters in file names.
        safe = "".join("_" if c in '&*/:`<>?\\|"' else c for c in name)
        png = download(LIBRETRO_THUMBS.format(system=DATS[system][0], name=urllib.parse.quote(safe)))
        # Revisions are often git symlinks, which raw.githubusercontent.com serves as the target's name.
        if png and not png.startswith(b"\x89PNG") and len(png) < 256 and png.endswith(b".png"):
            target = png.decode("utf-8", "replace")[:-4]
            png = download(LIBRETRO_THUMBS.format(system=DATS[system][0], name=urllib.parse.quote(target)))
        if png:
            return png
    return None


def fetch_ds(path, titles):
    code = game_code(path, 0x0C)
    if not code:
        return None, None
    for region in REGIONS.get(code[3], ["US", "EN"]):
        png = download(GAMETDB.format(region=region, code=code))
        if png:
            return png, code
    return fetch_thumbnail("nds", titles.get(code, [])), code


def strip_tags(stem):
    """Drops trailing (...) and [...] tags, like DSCore's titleFromFileName."""
    while stem[-1:] in (")", "]") and ("(" if stem[-1] == ")" else "[") in stem:
        stem = stem[:stem.rfind("(" if stem[-1] == ")" else "[")].rstrip()
    return stem


def name_guesses(path):
    stem = os.path.splitext(os.path.basename(path))[0]
    return [stem] + [strip_tags(stem) + suffix for suffix in REGION_SUFFIXES]


def fetch_gba(path, titles):
    code = game_code(path, 0xAC)
    return fetch_thumbnail("gba", titles.get(code, []) + name_guesses(path))


def crc32s(path):
    """CRC32 of the file, plus without the 16-byte iNES header or a 512-byte SNES copier header."""
    with open(path, "rb") as f:
        data = f.read()
    crcs = [f"{zlib.crc32(data):08X}"]
    if data[:4] == b"NES\x1a":
        crcs.append(f"{zlib.crc32(data[16:]):08X}")
    elif path.lower().endswith((".smc", ".sfc")) and len(data) % 1024 == 512:
        crcs.append(f"{zlib.crc32(data[512:]):08X}")
    return crcs


def same_title(path, titles):
    """No-Intro names whose title without tags equals the file's (e.g. for bad or hacked dumps)."""
    base = strip_tags(os.path.splitext(os.path.basename(path))[0]).lower()
    return sorted({n for names in titles.values() for n in names if strip_tags(n).lower() == base}, key=rank)


def fetch_by_crc(path, system, titles):
    names = [name for crc in crc32s(path) for name in titles.get(crc, [])]
    return fetch_thumbnail(system, names + name_guesses(path) + same_title(path, titles))


# --- Main -----------------------------------------------------------------------------------------

def list_roms(sd_root):
    for root, _, files in os.walk(os.path.join(sd_root, "roms")):
        for name in sorted(files):
            system = EXTENSIONS.get(os.path.splitext(name)[1].lower())
            if system and not name.startswith("."):
                yield os.path.join(root, name), system


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 1:
        sys.exit(__doc__)
    sd_root = args[0]
    save_twilight, force = "--twilight" in sys.argv, "--force" in sys.argv
    covers_dir = os.path.join(sd_root, "_nds", "DSCore", "covers")
    boxart_dir = os.path.join(sd_root, "_nds", "TWiLightMenu", "boxart")
    os.makedirs(covers_dir, exist_ok=True)
    if save_twilight:
        os.makedirs(boxart_dir, exist_ok=True)

    roms = list(list_roms(sd_root))
    titles = {system: load_titles(system) for system in sorted({system for _, system in roms})}
    found = skipped = 0
    missing = []
    for i, (path, system) in enumerate(roms, 1):
        name = os.path.basename(path)
        target = os.path.join(covers_dir, name + ".bin")
        if os.path.exists(target) and not force:
            skipped += 1
            continue
        twilight_name = name + ".png"
        if system == "nds":
            png, code = fetch_ds(path, titles["nds"])
            twilight_name = (code or name) + ".png"
        elif system == "gba":
            png = fetch_gba(path, titles["gba"])
        else:
            png = fetch_by_crc(path, system, titles[system])
        if not png:
            missing.append(name)
            print(f"[{i}/{len(roms)}] no cover: {name}")
            continue
        try:
            converted = to_dscore(png)
        except ValueError as error:
            missing.append(name)
            print(f"[{i}/{len(roms)}] unreadable cover ({error}): {name}")
            continue
        with open(target, "wb") as f:
            f.write(converted)
        if save_twilight:
            with open(os.path.join(boxart_dir, twilight_name), "wb") as f:
                f.write(png)
        found += 1
        print(f"[{i}/{len(roms)}] ok: {name}")
        time.sleep(0.1)  # be gentle with the servers

    print(f"\n{found} covers saved, {skipped} already present, {len(missing)} not found.")
    missing_list = os.path.join(covers_dir, "missing.txt")
    if missing:
        with open(missing_list, "w", encoding="utf-8") as f:
            f.write("\n".join(missing) + "\n")
        print(f"Missing list: {missing_list}")
    elif os.path.exists(missing_list):
        os.remove(missing_list)

    count = write_thumbs(covers_dir, [os.path.basename(path) for path, _ in roms],
                         os.path.join(sd_root, "_nds", "DSCore", "thumbs.bin"))
    print(f"Grid thumbnails: {count}")


if __name__ == "__main__":
    main()
