#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "core/IniPatch.h"

namespace dscore {

enum class RomKind { Nds, Gba, Unsupported };

RomKind romKindFor(std::string_view path);

// [SRLOADER] keys that make TWiLight Menu++'s main.srldr relaunch romPath from the SD card
// (see lastRunROM() in title/arm9/source/main.cpp): the same per-launch keys TWiLight's ROM browser
// writes. homebrew selects nds-bootstrap-hb for DS files and is ignored for GBA.
// Empty for unsupported files.
std::vector<IniKey> relaunchKeys(std::string_view romPath, bool homebrew);

// main.srldr only honours the auto-run bit at 0x02000000 when 0x02000004 holds its warm-relaunch
// marker; otherwise it treats the boot as cold and clears the bits. Older releases (v25.10.0) expect 0,
// newer ones (v27.24.1 and later) expect 'RSET'.
//
// True when main.srldr's ARM9 binary contains the 'RSET' constant, i.e. it uses the newer marker.
bool usesRsetMarker(const uint8_t* arm9, size_t len);

// Value to store at 0x02000004 before booting main.srldr.
uint32_t relaunchMarker(bool rset);

} // namespace dscore
