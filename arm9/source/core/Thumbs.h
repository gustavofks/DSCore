#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace dscore {

// thumbs.bin holds the small box art the grid shows, written by tools/fetch_covers.py:
//   header: "DSTH", version (u16), reserved (u16), entry count (u32)
//   entries, sorted by key: key (u32), pixel offset from the start of the file (u32), width (u8),
//   height (u8), reserved (u16)
//   pixels: width * height DS colors (u16, bit 15 set) per thumbnail
// All numbers are little-endian.

constexpr int kThumbMaxSize = 40;
constexpr size_t kThumbHeaderSize = 12;
constexpr size_t kThumbEntrySize = 12;

struct ThumbEntry {
	uint32_t key = 0;
	uint32_t offset = 0;
	uint8_t width = 0;
	uint8_t height = 0;
};

// Key of a game's thumbnail: FNV-1a of the ROM file name without folders (e.g. "Metroid (USA).nes").
uint32_t thumbKey(std::string_view romPath);

// Entry count from the header; false when it is not a thumbs.bin this version reads.
bool parseThumbHeader(const uint8_t* data, size_t len, uint32_t& count);

// Reads count entries; false when they are malformed (unsorted, too large or empty thumbnails).
bool parseThumbEntries(const uint8_t* data, size_t len, uint32_t count, std::vector<ThumbEntry>& out);

// The entry for key, or nullptr.
const ThumbEntry* findThumb(const std::vector<ThumbEntry>& entries, uint32_t key);

} // namespace dscore
