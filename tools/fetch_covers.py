#!/usr/bin/env python3
"""Downloads box art for the games on a DSi SD card and converts it for DSCore.

Usage: python tools/fetch_covers.py <sd-root> [--twilight] [--force]

  <sd-root>   SD card root (e.g. E:\\) or a copy of it; games are read from roms/NDS and roms/GBA.
  --twilight  Also save the original PNGs where TWiLight Menu++ looks for box art.
  --force     Download again covers that already exist.

DS covers come from GameTDB by game code; GBA covers from libretro-thumbnails by No-Intro file name.
Each cover is scaled to fit 112x112 and written to _nds/DSCore/covers/<rom file name>.bin in DSCore's
format: "DSCV", width and height (u16 LE), then width*height DS colors (u16 LE, bit 15 set).
Only the Python standard library is used.
"""
import os
import struct
import sys
import time
import urllib.parse
import urllib.request
import zlib

MAX_SIZE = 112
GAMETDB = "https://art.gametdb.com/ds/coverS/{region}/{code}.png"
LIBRETRO_GBA = "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/master/Named_Boxarts/{name}.png"
# GameTDB region folder from the last letter of a DS game code.
REGIONS = {"E": ["US"], "P": ["EN", "US"], "J": ["JA"], "K": ["KO"], "F": ["FR", "EN"], "D": ["DE", "EN"],
           "S": ["ES", "EN"], "I": ["IT", "EN"], "H": ["NL", "EN"], "U": ["AU", "EN"], "O": ["US", "EN"]}
GBA_REGION_SUFFIXES = ["", " (USA)", " (USA, Europe)", " (Europe)", " (World)", " (Japan)"]


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

def fit(width, height):
    scale = min(MAX_SIZE / width, MAX_SIZE / height, 1.0)
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


# --- Sources --------------------------------------------------------------------------------------

def download(url):
    request = urllib.request.Request(url, headers={"User-Agent": "DSCore cover fetcher"})
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            return response.read()
    except Exception:
        return None


def ds_game_code(path):
    with open(path, "rb") as f:
        header = f.read(0x10)
    code = header[0x0C:0x10].decode("ascii", "replace")
    return code if code.isalnum() and len(code) == 4 else None


def fetch_ds(path):
    code = ds_game_code(path)
    if not code:
        return None, None
    for region in REGIONS.get(code[3], ["US", "EN"]):
        png = download(GAMETDB.format(region=region, code=code))
        if png:
            return png, code
    return None, code


def strip_tags(stem):
    while stem.endswith(")") and "(" in stem:
        stem = stem[:stem.rfind("(")].rstrip()
    return stem


def fetch_gba(path):
    stem = os.path.splitext(os.path.basename(path))[0]
    names = [stem] + [strip_tags(stem) + suffix for suffix in GBA_REGION_SUFFIXES]
    for name in dict.fromkeys(names):  # unique, in order
        # libretro-thumbnails replaces these characters in file names.
        safe = "".join("_" if c in '&*/:`<>?\\|"' else c for c in name)
        png = download(LIBRETRO_GBA.format(name=urllib.parse.quote(safe)))
        if png:
            return png
    return None


# --- Main -----------------------------------------------------------------------------------------

def list_roms(sd_root):
    for folder, ext in (("roms/NDS", ".nds"), ("roms/GBA", ".gba")):
        for root, _, files in os.walk(os.path.join(sd_root, folder)):
            for name in sorted(files):
                if name.lower().endswith(ext) and not name.startswith("."):
                    yield os.path.join(root, name), ext


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

    found = skipped = 0
    missing = []
    roms = list(list_roms(sd_root))
    for i, (path, ext) in enumerate(roms, 1):
        name = os.path.basename(path)
        target = os.path.join(covers_dir, name + ".bin")
        if os.path.exists(target) and not force:
            skipped += 1
            continue
        if ext == ".nds":
            png, code = fetch_ds(path)
            twilight_name = (code or name) + ".png"
        else:
            png, twilight_name = fetch_gba(path), name + ".png"
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
    if missing:
        with open(os.path.join(covers_dir, "missing.txt"), "w", encoding="utf-8") as f:
            f.write("\n".join(missing) + "\n")
        print(f"Missing list: {os.path.join(covers_dir, 'missing.txt')}")


if __name__ == "__main__":
    main()
