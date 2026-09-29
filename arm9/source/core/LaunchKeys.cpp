#include "core/LaunchKeys.h"

#include <cstring>
#include <string>

namespace dscore {

namespace {

// TWLSettings::TLaunchType values (universal/include/common/twlmenusettings.h).
constexpr const char* kLaunchTypeSdFlashcard = "1"; // ESDFlashcardLaunch: DS games through nds-bootstrap
constexpr const char* kLaunchTypeNesDs = "4";       // ENESDSLaunch
constexpr const char* kLaunchTypeGameYob = "5";     // EGameYobLaunch
constexpr const char* kLaunchTypeS8Ds = "6";        // ES8DSLaunch
constexpr const char* kLaunchTypeStellaDs = "9";    // EStellaDSLaunch
constexpr const char* kLaunchTypeSnemulDs = "21";   // ESNEmulDSLaunch
constexpr const char* kGbaRunner2 = "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";

constexpr uint32_t kRsetMarker = 0x54455352; // 'RSET' as stored little-endian in memory
constexpr char kRsetBytes[] = {'R', 'S', 'E', 'T'};

// LAUNCH_TYPE for a system's games, or nullptr when lastRunROM() cannot relaunch them.
const char* launchTypeFor(System system) {
	switch (system) {
		case System::Nds:
		case System::Gba: return kLaunchTypeSdFlashcard;
		case System::Gb:
		case System::Gbc: return kLaunchTypeGameYob;
		case System::Nes: return kLaunchTypeNesDs;
		case System::Sms:
		case System::GameGear: return kLaunchTypeS8Ds;
		case System::Atari2600: return kLaunchTypeStellaDs;
		case System::Snes: return kLaunchTypeSnemulDs;
		default: return nullptr;
	}
}

} // namespace

std::vector<IniKey> relaunchKeys(std::string_view romPath, bool homebrew) {
	System system;
	if (!systemForPath(romPath, system) || !launchTypeFor(system)) return {};
	const bool emulated = twilightEmulator(system) != nullptr;
	const bool viaBootstrapHb = system != System::Nds || homebrew;
	std::vector<IniKey> keys = {
		{"ROM_PATH", std::string(romPath)},
		{"LAUNCH_TYPE", launchTypeFor(system)},
		{"PREVIOUS_USED_DEVICE", "0"},
		{"SLOT1_LAUNCHED", "0"}, // otherwise lastRunROM() boots the Slot-1 card instead
		{"HOMEBREW_BOOTSTRAP", viaBootstrapHb ? "1" : "0"},
	};
	// lastRunROM() passes HOMEBREW_ARG to emulators as argv[1]; for GBA the ROM goes to nds-bootstrap.ini.
	if (emulated) keys.push_back({"HOMEBREW_ARG", std::string(romPath)});
	else if (system == System::Gba) keys.push_back({"HOMEBREW_ARG", ""});
	return keys;
}

const char* twilightEmulator(System system) {
	switch (system) {
		case System::Gb:
		case System::Gbc: return "sd:/_nds/TWiLightMenu/emulators/gameyob.nds";
		case System::Nes: return "sd:/_nds/TWiLightMenu/emulators/nestwl.nds";
		case System::Sms:
		case System::GameGear: return "sd:/_nds/TWiLightMenu/emulators/S8DS.nds";
		case System::Atari2600: return "sd:/_nds/TWiLightMenu/emulators/StellaDS.nds";
		// lastRunROM() boots it through the ToolchainGenericDS loader and turns HOMEBREW_ARG into a fat: path.
		case System::Snes: return "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.srl";
		default: return nullptr;
	}
}

std::vector<IniKey> bootstrapKeys(std::string_view romPath) {
	System system;
	if (!systemForPath(romPath, system) || system != System::Gba) return {};
	std::string fatPath(romPath);
	if (fatPath.rfind("sd:/", 0) == 0) fatPath.replace(0, 3, "fat:");
	return {
		{"NDS_PATH", kGbaRunner2},
		{"HOMEBREW_ARG", fatPath},
		{"RAM_DRIVE_PATH", ""},
		{"DSI_MODE", "0"},
		{"BOOST_CPU", "1"},
		{"BOOST_VRAM", "0"},
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
