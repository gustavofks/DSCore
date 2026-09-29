#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace dscore {

constexpr int kIconSize = 32;
constexpr size_t kNdsIconBannerEnd = 0x240; // banner bytes needed for the icon

// DS banner icon: 32x32, 4 bpp in 8x8 tiles, 16-color RGB555 palette where index 0 is transparent.
struct NdsIcon {
	uint8_t bitmap[512];
	uint16_t palette[16];
};

// Copies the icon from a banner (bitmap at 0x20, palette at 0x220). False if the banner is too short.
bool readNdsIcon(const uint8_t* banner, size_t len, NdsIcon& out);

// Expands to kIconSize * kIconSize pixels, row-major, as ARGB1555: bit 15 set for opaque pixels and
// 0 for transparent ones.
void decodeNdsIcon(const NdsIcon& icon, uint16_t* pixels);

constexpr size_t kGbaHeaderEnd = 0xB0;

struct GbaHeaderInfo {
	std::string title;    // up to 12 characters from the header, e.g. "METROID4USA"
	std::string gameCode; // 4 characters, e.g. "AMTE"
};

bool parseGbaHeader(const uint8_t* rom, size_t len, GbaHeaderInfo& out);

// Display title from a ROM path: file name without extension and without trailing "(...)" / "[...]"
// tags such as regions and dump groups, with a No-Intro trailing article moved to the front ("Legend of
// Zelda, The" becomes "The Legend of Zelda"). Returns the bare name if stripping would leave nothing.
std::string titleFromFileName(std::string_view path);

// Two uppercase characters for a generated tile: the first letters of the first two significant words
// (of the subtitle after " - " when there is one), or the first two letters of a single word; "?" when
// the title has no letters. E.g. "MZ" for "Metroid Zero Mission", "ME" for "Classic NES Series - Metroid".
std::string initialsFor(std::string_view title);

} // namespace dscore
