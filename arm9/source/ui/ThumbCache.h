#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "core/Thumbs.h"

namespace dscore {

// Grid thumbnails of the most recently drawn games, read from thumbs.bin through a loader so that the
// UI stays free of file access. Fixed size, slots on the heap (like IconCache).
class ThumbCache {
public:
	static constexpr int kSlots = 64; // four grid pages

	// Reads entry's width * height pixels into out; false on a read error.
	using Loader = std::function<bool(const ThumbEntry& entry, uint16_t* out)>;

	ThumbCache() : slots_(kSlots) {}

	void setSource(std::vector<ThumbEntry> entries, Loader loader);
	bool empty() const { return entries_.empty(); }

	// Pixels of the game's thumbnail and its size, or nullptr when it has none or it cannot be read.
	const uint16_t* get(const std::string& romPath, int& width, int& height);

	// The accent color of the game's cover (see ThumbEntry), or 0 when unknown. Reads nothing from the card.
	uint16_t accent(const std::string& romPath) const;

private:
	struct Slot {
		uint32_t key = 0;
		bool used = false;
		uint32_t lastUse = 0;
		uint16_t pixels[kThumbMaxSize * kThumbMaxSize];
	};

	std::vector<ThumbEntry> entries_;
	Loader loader_;
	std::vector<Slot> slots_;
	std::vector<uint32_t> failed_; // keys whose read failed, not retried
	uint32_t clock_ = 0;
};

} // namespace dscore
