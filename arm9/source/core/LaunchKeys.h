#pragma once

#include <string_view>
#include <vector>

#include "core/IniPatch.h"

namespace dscore {

enum class RomKind { Nds, Gba, Unsupported };

RomKind romKindFor(std::string_view path);

// [SRLOADER] keys that make TWiLight Menu++'s main.srldr relaunch romPath from the SD card
// (see lastRunROM() in title/arm9/source/main.cpp). Empty for unsupported files.
std::vector<IniKey> relaunchKeys(std::string_view romPath);

} // namespace dscore
