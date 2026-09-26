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

std::vector<IniKey> relaunchKeys(std::string_view romPath) {
	const char* launchType = nullptr;
	switch (romKindFor(romPath)) {
		case RomKind::Nds: launchType = kLaunchTypeSdFlashcard; break;
		case RomKind::Gba: launchType = kLaunchTypeGbaRunner2; break;
		case RomKind::Unsupported: return {};
	}
	return {
		{"ROM_PATH", std::string(romPath)},
		{"LAUNCH_TYPE", launchType},
		{"PREVIOUS_USED_DEVICE", "0"},
	};
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
