#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/Library.h"
#include "core/RomMedia.h"

namespace dscore {

struct LibraryData {
	std::vector<GameEntry> games;
	std::vector<NdsIcon> icons;
};

// Binary library cache (library.bin): "DSCL" magic, format version, counts and an FNV-1a checksum of
// the payload, followed by the games and their icons. Little-endian throughout.
std::vector<uint8_t> encodeLibrary(const LibraryData& library);

// False, leaving out unspecified, when the data is truncated, from another format version, corrupted
// or inconsistent. Callers then rebuild the cache from the SD card.
bool decodeLibrary(const uint8_t* data, size_t len, LibraryData& out);

} // namespace dscore
