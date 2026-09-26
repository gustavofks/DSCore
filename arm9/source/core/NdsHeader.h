#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace dscore {

constexpr size_t kNdsHeaderSize = 0x200;
constexpr size_t kBannerTitlesEnd = 0x840; // end of the six titles every banner version has

enum class BannerLanguage : int { Japanese = 0, English = 1, French = 2, German = 3, Italian = 4, Spanish = 5 };

struct NdsHeaderInfo {
	std::string gameCode;      // 4 characters, e.g. "ASMA"
	uint32_t arm9Offset = 0;   // file offset of the ARM9 binary
	uint32_t arm9Size = 0;
	uint32_t bannerOffset = 0; // 0 when the ROM has no banner
};

// Returns false when len < kNdsHeaderSize.
bool parseNdsHeader(const uint8_t* header, size_t len, NdsHeaderInfo& out);

// First line of the banner title in lang. Empty when the banner is shorter than kBannerTitlesEnd,
// has version 0, or the title is empty.
std::string bannerTitle(const uint8_t* banner, size_t len, BannerLanguage lang);

} // namespace dscore
