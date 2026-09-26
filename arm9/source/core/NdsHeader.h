#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace dscore {

constexpr size_t kNdsHeaderSize = 0x200;
constexpr size_t kBannerTitlesEnd = 0x840; // end of the six titles every banner version has

enum class BannerLanguage : int { Japanese = 0, English = 1, French = 2, German = 3, Italian = 4, Spanish = 5 };

struct NdsHeaderInfo {
	std::string gameTitle;     // up to 12 characters, trailing NULs removed
	std::string gameCode;      // 4 characters, e.g. "ASMA"
	uint32_t arm9Offset = 0;   // file offset of the ARM9 binary
	uint32_t arm9Entry = 0;
	uint32_t arm9Ram = 0;      // address the ARM9 binary is loaded to
	uint32_t arm9Size = 0;
	uint32_t arm7Entry = 0;
	uint32_t arm7Ram = 0;
	uint32_t bannerOffset = 0; // 0 when the ROM has no banner
};

constexpr size_t kArm9StartSize = 16;

// Returns false when len < kNdsHeaderSize.
bool parseNdsHeader(const uint8_t* header, size_t len, NdsHeaderInfo& out);

// File offset of the first instruction the ARM9 executes.
uint32_t arm9EntryFileOffset(const NdsHeaderInfo& info);

// Same rules TWiLight Menu++ uses to flag a ROM as homebrew (romsel_dsimenutheme iconTitle.cpp):
// libnds entry code, a few special-cased titles, or an ARM7 binary loaded into IWRAM.
// arm9Start holds kArm9StartSize bytes read at arm9EntryFileOffset().
bool isHomebrew(const NdsHeaderInfo& info, const uint8_t* arm9Start);

// First line of the banner title in lang. Empty when the banner is shorter than kBannerTitlesEnd,
// has version 0, or the title is empty.
std::string bannerTitle(const uint8_t* banner, size_t len, BannerLanguage lang);

} // namespace dscore
