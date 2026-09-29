#include "ui/ThumbCache.h"

#include <algorithm>
#include <utility>

namespace dscore {

void ThumbCache::setSource(std::vector<ThumbEntry> entries, Loader loader) {
	entries_ = std::move(entries);
	loader_ = std::move(loader);
	failed_.clear();
	for (Slot& slot : slots_) slot.used = false;
}

const uint16_t* ThumbCache::get(const std::string& romPath, int& width, int& height) {
	if (entries_.empty()) return nullptr;
	const uint32_t key = thumbKey(romPath);
	const ThumbEntry* entry = findThumb(entries_, key);
	if (!entry || std::find(failed_.begin(), failed_.end(), key) != failed_.end()) return nullptr;
	width = entry->width;
	height = entry->height;

	Slot* victim = &slots_[0];
	for (Slot& slot : slots_) {
		if (slot.used && slot.key == key) {
			slot.lastUse = ++clock_;
			return slot.pixels;
		}
		if (!slot.used || (victim->used && slot.lastUse < victim->lastUse)) victim = &slot;
	}
	if (!loader_ || !loader_(*entry, victim->pixels)) {
		failed_.push_back(key);
		return nullptr;
	}
	victim->key = key;
	victim->used = true;
	victim->lastUse = ++clock_;
	return victim->pixels;
}

} // namespace dscore
