#include "launch/TwilightLauncher.h"

#include <nds.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "common/nds_loader_arm9.h"
#include "common/systemdetails.h"
#include "core/IniPatch.h"
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
	if (romKindFor(romPath) != RomKind::Nds) return true;
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

} // namespace

LaunchError launchViaTwilight(const std::string& romPath, int* loaderCode) {
	if (romKindFor(romPath) == RomKind::Unsupported) return LaunchError::Unsupported;
	bool homebrew = false;
	if (!detectHomebrew(romPath, homebrew)) return LaunchError::RomRead;
	const std::vector<IniKey> keys = relaunchKeys(romPath, homebrew);

	bool rset = false;
	if (!detectRsetMarker(rset)) return LaunchError::MainRead;

	if (!fileExists(kSettingsPath)) return LaunchError::SettingsRead;
	if (!patchIniFile(kSettingsPath, kSettingsBackup, "SRLOADER", keys)) return LaunchError::SettingsWrite;
	const std::vector<IniKey> bootstrap = bootstrapKeys(romPath);
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
		case LaunchError::MainRead: return "could not read TWiLight main.srldr";
		case LaunchError::SettingsRead: return "could not read TWiLight settings.ini";
		case LaunchError::SettingsWrite: return "could not write TWiLight's settings";
		case LaunchError::LoaderFailed: return "runNdsFile could not boot main.srldr";
	}
	return "unknown error";
}

} // namespace dscore
