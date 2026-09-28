#include "ui/IconCache.h"

namespace dscore {

void IconCache::clear() {
	for (Slot& slot : slots_) slot.index = -1;
}

const uint16_t* IconCache::get(const std::vector<NdsIcon>& icons, int index) {
	Slot* victim = &slots_[0];
	for (Slot& slot : slots_) {
		if (slot.index == index) {
			slot.lastUse = ++clock_;
			return slot.pixels;
		}
		if (slot.lastUse < victim->lastUse) victim = &slot;
	}
	decodeNdsIcon(icons[size_t(index)], victim->pixels);
	victim->index = index;
	victim->lastUse = ++clock_;
	return victim->pixels;
}

} // namespace dscore
