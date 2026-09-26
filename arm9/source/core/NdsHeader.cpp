#include "core/NdsHeader.h"

#include "core/Text.h"

namespace dscore {

namespace {

constexpr size_t kGameCodeOffset = 0x0C;
constexpr size_t kArm9OffsetField = 0x20;
constexpr size_t kArm9SizeField = 0x2C;
constexpr size_t kBannerOffsetField = 0x68;
constexpr size_t kTitlesOffset = 0x240;
constexpr size_t kTitleBytes = 0x100; // 128 UTF-16 code units

uint16_t readU16(const uint8_t* p) {
	return uint16_t(p[0] | (p[1] << 8));
}

uint32_t readU32(const uint8_t* p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24);
}

} // namespace

bool parseNdsHeader(const uint8_t* header, size_t len, NdsHeaderInfo& out) {
	if (len < kNdsHeaderSize) return false;
	out.gameCode.assign(reinterpret_cast<const char*>(header + kGameCodeOffset), 4);
	out.arm9Offset = readU32(header + kArm9OffsetField);
	out.arm9Size = readU32(header + kArm9SizeField);
	out.bannerOffset = readU32(header + kBannerOffsetField);
	return true;
}

std::string bannerTitle(const uint8_t* banner, size_t len, BannerLanguage lang) {
	if (len < kBannerTitlesEnd || readU16(banner) == 0) return {};
	const uint8_t* title = banner + kTitlesOffset + size_t(lang) * kTitleBytes;
	std::string text = utf16leToUtf8(title, kTitleBytes / 2);
	const size_t newline = text.find('\n');
	if (newline != std::string::npos) text.resize(newline);
	while (!text.empty() && (text.back() == ' ' || text.back() == '\r')) text.pop_back();
	return text;
}

} // namespace dscore
