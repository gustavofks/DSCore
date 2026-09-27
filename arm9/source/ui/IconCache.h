#pragma once

#include <cstdint>
#include <vector>

#include "core/RomMedia.h"

namespace dscore {

// Decoded ARGB1555 icons for the most recently drawn games, so redrawing a page does not untile the
// same icons again. Fixed size: memory does not grow with the library. The slots live on the heap
// (~100 KB), which keeps owners safe to put on the DS's small DTCM stack.
class IconCache {
public:
	static constexpr int kSlots = 48; // three grid pages

	IconCache() : slots_(kSlots) {}

	// Pixels (kIconSize x kIconSize) of icons[index], decoding it if it is not cached.
	const uint16_t* get(const std::vector<NdsIcon>& icons, int index);

private:
	struct Slot {
		int index = -1;
		uint32_t lastUse = 0;
		uint16_t pixels[kIconSize * kIconSize];
	};

	std::vector<Slot> slots_;
	uint32_t clock_ = 0;
};

} // namespace dscore
