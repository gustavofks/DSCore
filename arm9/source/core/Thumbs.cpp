#include "core/Thumbs.h"

#include <algorithm>
#include <cstring>

namespace dscore {

namespace {

constexpr char kMagic[4] = {'D', 'S', 'T', 'H'};
constexpr uint16_t kVersion = 1;

uint16_t u16(const uint8_t* p) {
	return uint16_t(p[0] | (p[1] << 8));
}

uint32_t u32(const uint8_t* p) {
	return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

} // namespace

uint32_t thumbKey(std::string_view romPath) {
	const size_t slash = romPath.find_last_of('/');
	const std::string_view name = slash == std::string_view::npos ? romPath : romPath.substr(slash + 1);
	uint32_t hash = 2166136261u;
	for (char c : name) hash = (hash ^ uint8_t(c)) * 16777619u;
	return hash;
}

bool parseThumbHeader(const uint8_t* data, size_t len, uint32_t& count) {
	if (len < kThumbHeaderSize || std::memcmp(data, kMagic, sizeof(kMagic)) != 0) return false;
	if (u16(data + 4) != kVersion) return false;
	count = u32(data + 8);
	return count <= 65535;
}

bool parseThumbEntries(const uint8_t* data, size_t len, uint32_t count, std::vector<ThumbEntry>& out) {
	if (len < size_t(count) * kThumbEntrySize) return false;
	std::vector<ThumbEntry> entries(count);
	for (uint32_t i = 0; i < count; ++i) {
		const uint8_t* p = data + size_t(i) * kThumbEntrySize;
		ThumbEntry& e = entries[i];
		e.key = u32(p);
		e.offset = u32(p + 4);
		e.width = p[8];
		e.height = p[9];
		e.accent = u16(p + 10);
		if (e.width == 0 || e.height == 0 || e.width > kThumbMaxSize || e.height > kThumbMaxSize) return false;
		if (i > 0 && entries[i - 1].key > e.key) return false;
	}
	out = std::move(entries);
	return true;
}

const ThumbEntry* findThumb(const std::vector<ThumbEntry>& entries, uint32_t key) {
	const auto it = std::lower_bound(entries.begin(), entries.end(), key,
		[](const ThumbEntry& e, uint32_t k) { return e.key < k; });
	return it != entries.end() && it->key == key ? &*it : nullptr;
}

} // namespace dscore
