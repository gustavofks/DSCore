#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/Library.h"
#include "core/NdsHeader.h"
#include "core/RomMedia.h"

namespace dscore {

// Every .nds / .gba file (any letter case) below the given folders, recursively, skipping hidden
// entries. Missing folders are ignored.
std::vector<std::string> listRomFiles(const std::vector<std::string>& roots);

// Reads what the library shows for one ROM: DS games take their title from the banner in lang (the
// file name when the banner has none) and set icon from it; GBA games use the file name as title.
// Matches the RomParser signature used by applyScan() once lang is bound.
bool readRomInfo(const std::string& path, BannerLanguage lang, GameEntry& game, std::optional<NdsIcon>& icon);

} // namespace dscore
