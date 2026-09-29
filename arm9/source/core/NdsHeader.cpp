#include "core/NdsHeader.h"

#include "core/Text.h"

namespace dscore {

namespace {

constexpr size_t kGameTitleSize = 12;
constexpr size_t kGameCodeOffset = 0x0C;
constexpr size_t kArm9OffsetField = 0x20;
constexpr size_t kArm9EntryField = 0x24;
constexpr size_t kArm9RamField = 0x28;
constexpr size_t kArm9SizeField = 0x2C;
constexpr size_t kArm7EntryField = 0x34;
constexpr size_t kArm7RamField = 0x38;
constexpr size_t kBannerOffsetField = 0x68;
constexpr size_t kTitlesOffset = 0x240;
constexpr size_t kTitleBytes = 0x100; // 128 UTF-16 code units

// ARM7 binaries of old homebrew run from shared/ARM7 WRAM instead of main RAM.
constexpr uint32_t kArm7IwramStart = 0x037F0000;

// libnds crt0: mov r0,#0x04000000 / str r0,[r0,#0x208] / mov r0,#0x13 / msr cpsr_c,r0
constexpr uint32_t kLibndsEntry[4] = {0xE3A00301, 0xE5800208, 0xE3A00013, 0xE129F000};

uint16_t readU16(const uint8_t* p) {
	return uint16_t(p[0] | (p[1] << 8));
}

uint32_t readU32(const uint8_t* p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24);
}

bool titleStartsWith(const std::string& title, const char* prefix) {
	return title.compare(0, std::char_traits<char>::length(prefix), prefix) == 0;
}

} // namespace

bool parseNdsHeader(const uint8_t* header, size_t len, NdsHeaderInfo& out) {
	if (len < kNdsHeaderSize) return false;
	out.gameTitle.assign(reinterpret_cast<const char*>(header), kGameTitleSize);
	out.gameTitle.resize(out.gameTitle.find_last_not_of('\0') + 1);
	out.gameCode.assign(reinterpret_cast<const char*>(header + kGameCodeOffset), 4);
	out.arm9Offset = readU32(header + kArm9OffsetField);
	out.arm9Entry = readU32(header + kArm9EntryField);
	out.arm9Ram = readU32(header + kArm9RamField);
	out.arm9Size = readU32(header + kArm9SizeField);
	out.arm7Entry = readU32(header + kArm7EntryField);
	out.arm7Ram = readU32(header + kArm7RamField);
	out.bannerOffset = readU32(header + kBannerOffsetField);
	return true;
}

uint32_t arm9EntryFileOffset(const NdsHeaderInfo& info) {
	return info.arm9Offset + info.arm9Entry - info.arm9Ram;
}

bool isHomebrew(const NdsHeaderInfo& info, const uint8_t* arm9Start) {
	bool libndsEntry = true;
	for (size_t i = 0; i < 4; ++i) libndsEntry = libndsEntry && readU32(arm9Start + 4 * i) == kLibndsEntry[i];
	if (libndsEntry) return true;

	if (titleStartsWith(info.gameTitle, "NDS.TinyFB") || titleStartsWith(info.gameTitle, "MAGIC FLOOR")
		|| titleStartsWith(info.gameTitle, "UNLAUNCH.DSI") || titleStartsWith(info.gameTitle, "NMP4BOOT")) {
		return true;
	}
	return info.arm7Entry >= kArm7IwramStart && info.arm7Ram >= kArm7IwramStart;
}

std::string bannerText(const uint8_t* banner, size_t len, BannerLanguage lang) {
	if (len < kBannerTitlesEnd || readU16(banner) == 0) return {};
	const uint8_t* title = banner + kTitlesOffset + size_t(lang) * kTitleBytes;
	std::string text = utf16leToUtf8(title, kTitleBytes / 2);
	while (!text.empty() && (text.back() == ' ' || text.back() == '\r' || text.back() == '\n')) text.pop_back();
	return text;
}

} // namespace dscore
