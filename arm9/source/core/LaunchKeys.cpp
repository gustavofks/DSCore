#include "core/LaunchKeys.h"

#include <string>

#include "core/Text.h"

namespace dscore {

namespace {

// TWLSettings::TLaunchType values (universal/include/common/twlmenusettings.h).
constexpr const char* kLaunchTypeSdFlashcard = "1"; // ESDFlashcardLaunch: DS games through nds-bootstrap
constexpr const char* kLaunchTypeGbaRunner2 = "17"; // EGBARunner2Launch

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

} // namespace dscore
