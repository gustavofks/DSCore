#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "core/LaunchKeys.h"
#include "core/LibraryCache.h"

namespace dscore {

// Fills game (everything but path and iconIndex) for the ROM at path and sets icon when the ROM has
// one. Returns false for unreadable files, which are left out of the library.
using RomParser = std::function<bool(const std::string& path, GameEntry& game, std::optional<NdsIcon>& icon)>;

// Brings the library in line with the ROM paths found on the SD card: keeps cached entries that still
// exist (in their cached order), parses only new paths, drops missing ones and compacts the icon list.
// Returns whether anything changed, i.e. whether the cache must be written again.
bool applyScan(LibraryData& library, const std::vector<std::string>& foundPaths, const RomParser& parse);

} // namespace dscore
