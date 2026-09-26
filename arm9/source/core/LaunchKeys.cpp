#include "core/LaunchKeys.h"

#include <cstring>
#include <string>

#include "core/Text.h"

namespace dscore {

namespace {

// TWLSettings::TLaunchType values (universal/include/common/twlmenusettings.h).
constexpr const char* kLaunchTypeSdFlashcard = "1"; // ESDFlashcardLaunch: DS games through nds-bootstrap
constexpr const char* kLaunchTypeGbaRunner2 = "17"; // EGBARunner2Launch

constexpr uint32_t kRsetMarker = 0x54455352; // 'RSET' as stored little-endian in memory
constexpr char kRsetBytes[] = {'R', 'S', 'E', 'T'};

} // namespace

RomKind romKindFor(std::string_view path) {
	if (hasExtension(path, ".nds")) return RomKind::Nds;
	if (hasExtension(path, ".gba")) return RomKind::Gba;
	return RomKind::Unsupported;
}

std::vector<IniKey> relaunchKeys(std::string_view romPath, bool homebrew) {
	const RomKind kind = romKindFor(romPath);
	if (kind == RomKind::Unsupported) return {};

	std::vector<IniKey> keys = {
		{"ROM_PATH", std::string(romPath)},
		{"LAUNCH_TYPE", kind == RomKind::Nds ? kLaunchTypeSdFlashcard : kLaunchTypeGbaRunner2},
		{"PREVIOUS_USED_DEVICE", "0"},
		{"SLOT1_LAUNCHED", "0"}, // otherwise lastRunROM() boots the Slot-1 card instead
	};
	if (kind == RomKind::Nds) {
		keys.push_back({"HOMEBREW_BOOTSTRAP", homebrew ? "1" : "0"});
	} else {
		keys.push_back({"HOMEBREW_ARG", std::string(romPath)}); // the ROM GBARunner2 is given
	}
	return keys;
}

bool usesRsetMarker(const uint8_t* arm9, size_t len) {
	if (len < sizeof(kRsetBytes)) return false;
	for (size_t i = 0; i + sizeof(kRsetBytes) <= len; ++i) {
		if (std::memcmp(arm9 + i, kRsetBytes, sizeof(kRsetBytes)) == 0) return true;
	}
	return false;
}

uint32_t relaunchMarker(bool rset) {
	return rset ? kRsetMarker : 0;
}

} // namespace dscore
