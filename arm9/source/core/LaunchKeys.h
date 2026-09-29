#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/IniPatch.h"
#include "core/Systems.h"

namespace dscore {

// [SRLOADER] keys (settings.ini) that make TWiLight Menu++'s main.srldr relaunch romPath from the SD
// card (see lastRunROM() in title/arm9/source/main.cpp): the same per-launch keys TWiLight's ROM
// browser writes. homebrew selects nds-bootstrap-hb for DS files and is ignored otherwise: GBA always
// relaunches through nds-bootstrap-hb, other consoles boot their emulator with the ROM as argument.
// Empty for unsupported files.
std::vector<IniKey> relaunchKeys(std::string_view romPath, bool homebrew);

// [NDS-BOOTSTRAP] keys (nds-bootstrap.ini) the relaunch needs besides settings.ini. TWiLight writes
// them for DS games itself, but for GBA its ROM browser sets them once at launch and the relaunch
// reuses them: nds-bootstrap-hb boots GBARunner2 with the ROM (as a "fat:" path) as argument.
// Empty for DS games.
std::vector<IniKey> bootstrapKeys(std::string_view romPath);

// The TWiLight Menu++ emulator main.srldr boots for system's games in DSi mode, or nullptr for DS and
// GBA games, which go through nds-bootstrap.
const char* twilightEmulator(System system);

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
