#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/Library.h"

namespace dscore {

// Facts about games that ROM files do not carry, read from sd:/_nds/DSCore/metadata.ini. A PC tool
// writes it (tools/fetch_metadata.py, or another program that follows the format): one section per ROM
// file name, without folders, e.g.
//
//   [Metroid Fusion (USA).gba]
//   genre = Action
//   year = 2002
//   developer = Nintendo
//   players = 1
//
// Every key is optional and unknown keys are ignored, so the format can grow.
struct GameMeta {
	std::string genre;
	std::string developer;
	uint16_t year = 0;   // 0 when unknown
	uint8_t players = 0; // most players at once, 0 when unknown
};

using MetadataMap = std::map<std::string, GameMeta, std::less<>>; // by ROM file name

MetadataMap parseMetadata(std::string_view ini);

// Copies the metadata of each game, matched by file name, into games (clearing the others); returns how
// many games matched.
size_t applyMetadata(const MetadataMap& metadata, std::vector<GameEntry>& games);

// The genres of games, sorted, without repeats.
std::vector<std::string> genresOf(const std::vector<GameEntry>& games);

} // namespace dscore
