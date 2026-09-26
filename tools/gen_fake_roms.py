#!/usr/bin/env python3
"""Writes tiny fake .nds files (valid header, banner title and icon, no game code) for stress tests.

Usage: tools/gen_fake_roms.py <out-dir> <count>

Each file is a few KB, so thousands of them fit anywhere. DSCore lists, indexes and draws them like real
games; launching one does nothing useful.
"""
import os
import struct
import sys

BANNER_OFFSET = 0x200
BANNER_SIZE = 0x840
WORDS = ["Star", "Quest", "Dragon", "Racer", "Puzzle", "Island", "Legend", "Robot", "Ninja", "Castle",
         "Galaxy", "Party", "Soccer", "Magic", "Ocean", "Shadow", "Crystal", "Turbo", "Jungle", "Pixel"]


def banner(index):
    title = f"{WORDS[index % len(WORDS)]} {WORDS[(index // len(WORDS)) % len(WORDS)]} {index:04d}\nFake Games"
    data = bytearray(BANNER_SIZE)
    struct.pack_into("<H", data, 0, 1)  # banner version
    # 4bpp icon: diagonal stripes whose colors depend on the index
    for tile in range(16):
        for row in range(8):
            for pair in range(4):
                a = (tile + row + index) % 15 + 1
                b = (tile + row + pair + index // 3) % 15 + 1
                data[0x20 + tile * 32 + row * 4 + pair] = a | (b << 4)
    for color in range(16):
        r, g, b = (index * 7 + color * 3) % 32, (color * 5) % 32, (index + color * 11) % 32
        struct.pack_into("<H", data, 0x220 + color * 2, r | (g << 5) | (b << 10))
    encoded = title.encode("utf-16-le")
    for lang in range(6):
        data[0x240 + lang * 0x100:0x240 + lang * 0x100 + len(encoded)] = encoded
    return bytes(data)


def rom(index):
    header = bytearray(0x200)
    header[0:12] = b"FAKEGAME".ljust(12, b"\0")
    header[0x0C:0x10] = b"ZZZZ"
    struct.pack_into("<I", header, 0x20, BANNER_OFFSET + BANNER_SIZE)  # ARM9 offset (not a real binary)
    struct.pack_into("<I", header, 0x24, 0x02000000)
    struct.pack_into("<I", header, 0x28, 0x02000000)
    struct.pack_into("<I", header, 0x2C, 16)
    struct.pack_into("<I", header, 0x34, 0x02380000)
    struct.pack_into("<I", header, 0x38, 0x02380000)
    struct.pack_into("<I", header, 0x68, BANNER_OFFSET)
    return bytes(header) + banner(index) + bytes(16)


def main():
    out_dir, count = sys.argv[1], int(sys.argv[2])
    os.makedirs(out_dir, exist_ok=True)
    for i in range(count):
        with open(os.path.join(out_dir, f"Fake Game {i:04d}.nds"), "wb") as f:
            f.write(rom(i))
    print(f"wrote {count} fake ROMs to {out_dir}")


if __name__ == "__main__":
    main()
