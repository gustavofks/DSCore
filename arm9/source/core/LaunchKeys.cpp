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
constexpr const char* kLaunchTypePicoDrive = "10";  // EPicoDriveTWLLaunch
constexpr const char* kLaunchTypeSnemulDs = "21";   // ESNEmulDSLaunch
constexpr const char* kGbaRunner2 = "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";
constexpr const char* kJenesis = "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
constexpr const char* kPicoDrive = "sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds";

constexpr uint32_t kRsetMarker = 0x54455352; // 'RSET' as stored little-endian in memory
constexpr char kRsetBytes[] = {'R', 'S', 'E', 'T'};

bool usesJenesis(System system, uint32_t romSize) {
	return system == System::MegaDrive && romSize <= kJenesisMaxSize;
}

// LAUNCH_TYPE for a system's games, or nullptr when lastRunROM() cannot relaunch them.
const char* launchTypeFor(System system, uint32_t romSize) {
	switch (system) {
		case System::Nds:
		case System::Gba: return kLaunchTypeSdFlashcard;
		case System::MegaDrive: return usesJenesis(system, romSize) ? kLaunchTypeSdFlashcard : kLaunchTypePicoDrive;
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

std::vector<IniKey> relaunchKeys(std::string_view romPath, bool homebrew, uint32_t romSize) {
	System system;
	if (!systemForPath(romPath, system) || !launchTypeFor(system, romSize)) return {};
	const bool viaBootstrapHb = system != System::Nds || homebrew;
	std::vector<IniKey> keys = {
		{"ROM_PATH", std::string(romPath)},
		{"LAUNCH_TYPE", launchTypeFor(system, romSize)},
		{"PREVIOUS_USED_DEVICE", "0"},
		{"SLOT1_LAUNCHED", "0"}, // otherwise lastRunROM() boots the Slot-1 card instead
		{"HOMEBREW_BOOTSTRAP", viaBootstrapHb ? "1" : "0"},
	};
	// lastRunROM() passes HOMEBREW_ARG to emulators as argv[1]; through nds-bootstrap-hb the ROM goes to
	// nds-bootstrap.ini instead.
	if (system == System::Gba || usesJenesis(system, romSize)) keys.push_back({"HOMEBREW_ARG", ""});
	else if (system != System::Nds) keys.push_back({"HOMEBREW_ARG", std::string(romPath)});
	if (system == System::MegaDrive && !usesJenesis(system, romSize)) keys.push_back({"SHOW_MDGEN", "1"});
	return keys;
}

std::vector<std::string> temporaryKeys(std::string_view romPath, uint32_t romSize) {
	System system;
	if (!systemForPath(romPath, system) || system != System::MegaDrive || usesJenesis(system, romSize)) return {};
	return {"SHOW_MDGEN"};
}

const char* emulatorFor(std::string_view romPath, uint32_t romSize) {
	System system;
	if (!systemForPath(romPath, system)) return nullptr;
	switch (system) {
		case System::Gba: return kGbaRunner2;
		case System::MegaDrive: return usesJenesis(system, romSize) ? kJenesis : kPicoDrive;
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

std::vector<IniKey> bootstrapKeys(std::string_view romPath, uint32_t romSize) {
	System system;
	if (!systemForPath(romPath, system)) return {};
	if (usesJenesis(system, romSize)) {
		// romToRamDisk: nds-bootstrap-hb loads the ROM into memory and jEnesisDS opens it as fat:/ROM.BIN.
		return {
			{"NDS_PATH", kJenesis},
			{"HOMEBREW_ARG", "fat:/ROM.BIN"},
			{"RAM_DRIVE_PATH", std::string(romPath)},
			{"DSI_MODE", "0"},
			{"BOOST_CPU", "1"},
			{"BOOST_VRAM", "0"},
		};
	}
	if (system != System::Gba) return {};
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

std::vector<std::string> saveFileCandidates(std::string_view romPath) {
	const size_t slash = romPath.find_last_of('/');
	const std::string dir(romPath.substr(0, slash == std::string_view::npos ? 0 : slash + 1));
	std::string_view name = romPath.substr(slash == std::string_view::npos ? 0 : slash + 1);
	const size_t dot = name.find_last_of('.');
	const std::string stem(dot == std::string_view::npos ? name : name.substr(0, dot));
	System system;
	if (!systemForPath(romPath, system)) return {};
	if (system == System::Nds) return {dir + "saves/" + stem + ".sav"};
	if (system == System::Snes) return {dir + stem + ".srm", dir + stem + ".sav"};
	return {dir + stem + ".sav"};
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
