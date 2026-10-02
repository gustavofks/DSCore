#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/IniPatch.h"
#include "core/Systems.h"

namespace dscore {

// .gen games up to this size run in jEnesisDS, like TWiLight Menu++'s "hybrid" setting; larger ones and
// .md files (which nds-bootstrap-hb cannot hand to jEnesisDS) run in PicoDriveTWL.
constexpr uint32_t kJenesisMaxSize = 0x300000;

// [SRLOADER] keys (settings.ini) that make TWiLight Menu++'s main.srldr relaunch romPath from the SD
// card (see lastRunROM() in title/arm9/source/main.cpp): the same per-launch keys TWiLight's ROM
// browser writes. homebrew selects nds-bootstrap-hb for DS files and is ignored otherwise: GBA, small
// .gen and (by default) SNES games relaunch through nds-bootstrap-hb, other consoles boot their
// emulator with the ROM as argument. romSize (bytes) picks the Mega Drive emulator; newSnesEmulator is
// TWiLight's NEW_SNES_EMU_VER setting. Empty for unsupported files.
std::vector<IniKey> relaunchKeys(std::string_view romPath, bool homebrew, uint32_t romSize = 0,
	bool newSnesEmulator = false);

// [NDS-BOOTSTRAP] keys (nds-bootstrap.ini) the relaunch needs besides settings.ini. TWiLight writes
// them for DS games itself, but for GBA and jEnesisDS its ROM browser sets them once at launch and the
// relaunch reuses them: nds-bootstrap-hb boots GBARunner2 with the ROM (as a "fat:" path) as argument,
// or jEnesisDS / SNEmulDS-legacy with the ROM loaded into a RAM drive. Empty for the other games.
std::vector<IniKey> bootstrapKeys(std::string_view romPath, uint32_t romSize = 0, bool newSnesEmulator = false);

// The emulator a game runs in (booted by main.srldr, or by nds-bootstrap-hb for GBA and small Mega
// Drive games), or nullptr for DS games. The launch fails without it, so DSCore checks it first.
const char* emulatorFor(std::string_view romPath, uint32_t romSize = 0, bool newSnesEmulator = false);

// [SRLOADER] keys that relaunchKeys() changes for this launch only; DSCore puts their old values back
// on its next start. v25.10.0's lastRunROM() only relaunches PicoDriveTWL when SHOW_MDGEN (the Mega
// Drive emulator setting) is below 2, while its default is 3 ("hybrid").
std::vector<std::string> temporaryKeys(std::string_view romPath, uint32_t romSize = 0);

// Where a game's save may be: nds-bootstrap keeps DS saves in a "saves" folder next to the ROM, GBARunner2
// and the emulators write <name>.sav (SNEmulDS <name>.srm) next to it.
std::vector<std::string> saveFileCandidates(std::string_view romPath);

// main.srldr only honours the auto-run bit at 0x02000000 when 0x02000004 holds its warm-relaunch
// marker; otherwise it treats the boot as cold and clears the bits. Older releases (v25.10.0) expect 0,
// newer ones (v27.24.1 and later) expect 'RSET'.
//
// True when main.srldr's ARM9 binary contains the 'RSET' constant, i.e. it uses the newer marker.
bool usesRsetMarker(const uint8_t* arm9, size_t len);

// Value to store at 0x02000004 before booting main.srldr.
uint32_t relaunchMarker(bool rset);

} // namespace dscore
