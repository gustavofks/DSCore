#include "launch/TwilightLauncher.h"

#include <nds.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "common/nds_loader_arm9.h"
#include "common/systemdetails.h"
#include "core/IniPatch.h"
#include "core/LaunchKeys.h"
#include "core/NdsHeader.h"

namespace dscore {

namespace {

constexpr const char* kSettingsPath = "sd:/_nds/TWiLightMenu/settings.ini";
constexpr const char* kSettingsBackup = "sd:/_nds/TWiLightMenu/settings.ini.dscore-bak";
constexpr const char* kSettingsTemp = "sd:/_nds/TWiLightMenu/settings.ini.dscore-tmp";
constexpr const char* kMainSrldr = "sd:/_nds/TWiLightMenu/main.srldr";

// title/arm9/source/main.cpp calls lastRunROM() when this bit is set and kRelaunchMarker holds the
// value it expects; the menu bootloader keeps 0x02000000-0x02003FFF intact across runNdsFile().
constexpr u32 kAutoRunBit = BIT(3);
vu32& softResetParams = *(vu32*)0x02000000;
vu32& relaunchMarkerSlot = *(vu32*)0x02000004;

constexpr size_t kScanChunk = 16 * 1024;
constexpr size_t kMarkerOverlap = 3; // keeps a constant split across two chunks findable

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

bool readFile(const char* path, std::string& out) {
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	char buffer[1024];
	size_t n;
	out.clear();
	while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) out.append(buffer, n);
	const bool ok = !ferror(f);
	fclose(f);
	return ok;
}

bool writeFile(const char* path, const std::string& data) {
	FILE* f = fopen(path, "wb");
	if (!f) return false;
	const bool written = fwrite(data.data(), 1, data.size(), f) == data.size();
	const bool closed = fclose(f) == 0;
	return written && closed;
}

// Writes to a temporary file first so a failed write never truncates settings.ini.
bool replaceFile(const char* path, const std::string& data) {
	if (!writeFile(kSettingsTemp, data)) return false;
	if (remove(path) == 0 && rename(kSettingsTemp, path) == 0) return true;
	// FAT rename can fail after the remove; fall back to writing the file directly.
	const bool ok = writeFile(path, data);
	remove(kSettingsTemp);
	return ok;
}

} // namespace

LaunchError launchViaTwilight(const std::string& romPath, int* loaderCode) {
	const std::vector<IniKey> keys = relaunchKeys(romPath);
	if (keys.empty()) return LaunchError::Unsupported;

	bool rset = false;
	if (!detectRsetMarker(rset)) return LaunchError::MainRead;

	std::string ini;
	if (!readFile(kSettingsPath, ini)) return LaunchError::SettingsRead;
	if (access(kSettingsBackup, F_OK) != 0 && !writeFile(kSettingsBackup, ini)) return LaunchError::SettingsWrite;
	if (!replaceFile(kSettingsPath, patchIni(ini, "SRLOADER", keys))) return LaunchError::SettingsWrite;

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
		case LaunchError::MainRead: return "could not read TWiLight main.srldr";
		case LaunchError::SettingsRead: return "could not read TWiLight settings.ini";
		case LaunchError::SettingsWrite: return "could not write TWiLight settings.ini";
		case LaunchError::LoaderFailed: return "runNdsFile could not boot main.srldr";
	}
	return "unknown error";
}

} // namespace dscore
