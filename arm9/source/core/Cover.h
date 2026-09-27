#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dscore {

// Box art prepared by tools/fetch_covers.py: "DSCV", width and height (u16 LE), then width * height DS
// colors (u16 LE, bit 15 set). Stored as covers/<rom file name>.bin in DSCore's data folder.
struct Cover {
	int width = 0;
	int height = 0;
	std::vector<uint16_t> pixels;
};

constexpr int kMaxCoverSize = 128;

// Empty when the data is not a valid cover (wrong magic, size over kMaxCoverSize or truncated).
std::optional<Cover> decodeCover(const uint8_t* data, size_t len);

// Cover file name for a ROM path, e.g. "Game.nds.bin" for "sd:/roms/NDS/Sub/Game.nds".
std::string coverFileName(const std::string& romPath);

} // namespace dscore
