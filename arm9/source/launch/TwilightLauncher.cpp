#include "launch/TwilightLauncher.h"

#include <nds.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "common/nds_loader_arm9.h"
#include "common/systemdetails.h"
#include "core/IniPatch.h"
#include "core/IniText.h"
#include "core/LaunchKeys.h"
#include "core/NdsHeader.h"
#include "platform/FileIo.h"

namespace dscore {

namespace {

constexpr const char* kSettingsPath = "sd:/_nds/TWiLightMenu/settings.ini";
constexpr const char* kSettingsBackup = "sd:/_nds/TWiLightMenu/settings.ini.dscore-bak";
constexpr const char* kBootstrapPath = "sd:/_nds/nds-bootstrap.ini";
constexpr const char* kBootstrapBackup = "sd:/_nds/nds-bootstrap.ini.dscore-bak";
constexpr const char* kMainSrldr = "sd:/_nds/TWiLightMenu/main.srldr";
// settings.ini values a launch changed for itself only (see temporaryKeys()).
constexpr const char* kRestorePath = "sd:/_nds/DSCore/restore-settings.ini";

// title/arm9/source/main.cpp calls lastRunROM() when this bit is set and kRelaunchMarker holds the
// value it expects; the menu bootloader keeps 0x02000000-0x02003FFF intact across runNdsFile().
constexpr u32 kAutoRunBit = BIT(3);
vu32& softResetParams = *(vu32*)0x02000000;
vu32& relaunchMarkerSlot = *(vu32*)0x02000004;

constexpr size_t kScanChunk = 16 * 1024;
constexpr size_t kMarkerOverlap = 3; // keeps a constant split across two chunks findable

// Classifies a .nds file the way TWiLight's ROM browser does. Non-DS files are never homebrew here.
bool detectHomebrew(const std::string& romPath, bool& homebrew) {
	homebrew = false;
	System system;
	if (!systemForPath(romPath, system) || system != System::Nds) return true;
	FILE* f = fopen(romPath.c_str(), "rb");
	if (!f) return false;
	uint8_t header[kNdsHeaderSize];
	uint8_t arm9Start[kArm9StartSize];
	NdsHeaderInfo info;
	const bool ok = fread(header, 1, sizeof(header), f) == sizeof(header)
		&& parseNdsHeader(header, sizeof(header), info)
		&& fseek(f, arm9EntryFileOffset(info), SEEK_SET) == 0
		&& fread(arm9Start, 1, sizeof(arm9Start), f) == sizeof(arm9Start);
	fclose(f);
	if (ok) homebrew = isHomebrew(info, arm9Start);
	return ok;
}

// Reads main.srldr's ARM9 binary in chunks and reports whether it uses the 'RSET' marker.
bool detectRsetMarker(bool& rset) {
	FILE* f = fopen(kMainSrldr, "rb");
	if (!f) return false;
	uint8_t header[kNdsHeaderSize];
	NdsHeaderInfo info;
	bool ok = fread(header, 1, sizeof(header), f) == sizeof(header)
		&& parseNdsHeader(header, sizeof(header), info)
		&& fseek(f, info.arm9Offset, SEEK_SET) == 0;

	static uint8_t chunk[kMarkerOverlap + kScanChunk];
	size_t carry = 0;
	uint32_t remaining = info.arm9Size;
	rset = false;
	while (ok && remaining > 0 && !rset) {
		const size_t want = std::min<size_t>(kScanChunk, remaining);
		if (fread(chunk + carry, 1, want, f) != want) {
			ok = false;
			break;
		}
		remaining -= want;
		const size_t total = carry + want;
		rset = usesRsetMarker(chunk, total);
		carry = std::min(kMarkerOverlap, total);
		std::memmove(chunk, chunk + total - carry, carry);
	}
	fclose(f);
	return ok;
}

// Sets keys in one [section] of a TWiLight INI file, backing the file up the first time DSCore edits it.
bool patchIniFile(const char* path, const char* backup, const char* section, const std::vector<IniKey>& keys) {
	std::string ini;
	if (!readFile(path, ini)) return false;
	if (!fileExists(backup) && !writeFile(backup, ini.data(), ini.size())) return false;
	const std::string patched = patchIni(ini, section, keys);
	return replaceFile(path, patched.data(), patched.size());
}

uint32_t fileSizeOf(const std::string& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0 && st.st_size > 0 ? uint32_t(st.st_size) : 0;
}

// True when settings.ini's [SRLOADER] key is a non-zero number; TWiLight's default is off.
bool settingIsOn(const char* wanted) {
	std::string ini;
	if (!readFile(kSettingsPath, ini)) return false;
	bool on = false;
	forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view value) {
		uint32_t number = 0;
		if (section == "SRLOADER" && key == wanted && parseIniUint(value, number)) on = number != 0;
	});
	return on;
}

// Saves the current values of keys (settings.ini, [SRLOADER]) so restoreTwilightSettings() can put them
// back. A restore file left by an earlier launch already holds the user's values and is kept.
bool rememberSettings(const std::vector<std::string>& keys) {
	if (keys.empty() || fileExists(kRestorePath)) return true;
	std::string ini;
	if (!readFile(kSettingsPath, ini)) return false;
	std::string out = "[SRLOADER]\n";
	for (const std::string& wanted : keys) {
		std::string value;
		forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view v) {
			if (section == "SRLOADER" && key == wanted) value = std::string(v);
		});
		out += wanted + " = " + value + "\n";
	}
	return writeFile(kRestorePath, out.data(), out.size());
}

} // namespace

void restoreTwilightSettings() {
	std::string ini;
	if (!readFile(kRestorePath, ini)) return;
	std::vector<IniKey> keys;
	forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view value) {
		if (section == "SRLOADER") keys.push_back({std::string(key), std::string(value)});
	});
	if (keys.empty() || patchIniFile(kSettingsPath, kSettingsBackup, "SRLOADER", keys)) remove(kRestorePath);
}

LaunchError launchViaTwilight(const std::string& romPath, int* loaderCode) {
	System system;
	if (!systemForPath(romPath, system)) return LaunchError::Unsupported;
	bool homebrew = false;
	if (!detectHomebrew(romPath, homebrew)) return LaunchError::RomRead;
	const uint32_t romSize = fileSizeOf(romPath);
	const bool newSnes = settingIsOn("NEW_SNES_EMU_VER");
	const std::vector<IniKey> keys = relaunchKeys(romPath, homebrew, romSize, newSnes);
	if (keys.empty()) return LaunchError::Unsupported;
	// Without its emulator, main.srldr would fall back to a flashcard path and fail after DSCore has quit.
	const char* emulator = emulatorFor(romPath, romSize, newSnes);
	if (emulator && !fileExists(emulator)) return LaunchError::EmulatorMissing;

	bool rset = false;
	if (!detectRsetMarker(rset)) return LaunchError::MainRead;

	if (!fileExists(kSettingsPath)) return LaunchError::SettingsRead;
	if (!rememberSettings(temporaryKeys(romPath, romSize))) return LaunchError::SettingsWrite;
	if (!patchIniFile(kSettingsPath, kSettingsBackup, "SRLOADER", keys)) return LaunchError::SettingsWrite;
	const std::vector<IniKey> bootstrap = bootstrapKeys(romPath, romSize, newSnes);
	if (!bootstrap.empty() && !patchIniFile(kBootstrapPath, kBootstrapBackup, "NDS-BOOTSTRAP", bootstrap)) {
		return LaunchError::SettingsWrite;
	}

	const u32 previousMarker = relaunchMarkerSlot;
	softResetParams |= kAutoRunBit;
	relaunchMarkerSlot = relaunchMarker(rset);
	const char* argv[] = {kMainSrldr};
	// Same arguments TWiLight's imageview uses to return to the menu.
	const int err = runNdsFile(kMainSrldr, 1, argv, sys().isRunFromSD(), true, false, false, true, true, false, -1);
	softResetParams &= ~kAutoRunBit;
	relaunchMarkerSlot = previousMarker;

	if (loaderCode) *loaderCode = err;
	return LaunchError::LoaderFailed;
}

const char* describe(LaunchError error) {
	switch (error) {
		case LaunchError::None: return "ok";
		case LaunchError::Unsupported: return "unsupported file type";
		case LaunchError::RomRead: return "could not read the ROM header";
		case LaunchError::EmulatorMissing: return "emulator missing in _nds/TWiLightMenu/emulators";
		case LaunchError::MainRead: return "could not read TWiLight main.srldr";
		case LaunchError::SettingsRead: return "could not read TWiLight settings.ini";
		case LaunchError::SettingsWrite: return "could not write TWiLight's settings";
		case LaunchError::LoaderFailed: return "runNdsFile could not boot main.srldr";
	}
	return "unknown error";
}

} // namespace dscore
