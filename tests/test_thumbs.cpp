#include "doctest.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "core/Thumbs.h"
#include "ui/ThumbCache.h"

using namespace dscore;

namespace {

void put32(std::vector<uint8_t>& out, uint32_t v) {
	for (int i = 0; i < 4; ++i) out.push_back(uint8_t(v >> (8 * i)));
}

// thumbs.bin with one w x h thumbnail per name, every pixel set to its index + 1.
std::vector<uint8_t> thumbsFile(const std::vector<std::string>& names, int w, int h) {
	std::vector<uint32_t> keys;
	for (const std::string& name : names) keys.push_back(thumbKey(name));
	std::sort(keys.begin(), keys.end());
	std::vector<uint8_t> out = {'D', 'S', 'T', 'H', 1, 0, 0, 0};
	put32(out, uint32_t(keys.size()));
	uint32_t offset = uint32_t(kThumbHeaderSize + kThumbEntrySize * keys.size());
	for (uint32_t key : keys) {
		put32(out, key);
		put32(out, offset);
		out.push_back(uint8_t(w));
		out.push_back(uint8_t(h));
		out.push_back(0x1F); // accent: rgb(31, 0, 0) with bit 15
		out.push_back(0x80);
		offset += uint32_t(w * h * 2);
	}
	for (size_t i = 0; i < keys.size(); ++i) {
		for (int p = 0; p < w * h; ++p) {
			out.push_back(uint8_t(i + 1));
			out.push_back(0x80);
		}
	}
	return out;
}

} // namespace

TEST_CASE("thumbKey hashes the file name only") {
	CHECK(thumbKey("sd:/roms/NES/Metroid (USA).nes") == thumbKey("Metroid (USA).nes"));
	CHECK(thumbKey("sd:/roms/NES/Metroid (USA).nes") != thumbKey("sd:/roms/NES/Metroid (Europe).nes"));
	CHECK(thumbKey("") == 2166136261u);
}

TEST_CASE("thumbs.bin header and entries parse and are searchable") {
	const std::vector<uint8_t> file = thumbsFile({"a.gb", "b.gbc", "c.nes"}, 30, 40);
	uint32_t count = 0;
	REQUIRE(parseThumbHeader(file.data(), file.size(), count));
	CHECK(count == 3);
	std::vector<ThumbEntry> entries;
	REQUIRE(parseThumbEntries(file.data() + kThumbHeaderSize, file.size() - kThumbHeaderSize, count, entries));
	const ThumbEntry* b = findThumb(entries, thumbKey("sd:/roms/GBC/b.gbc"));
	REQUIRE(b != nullptr);
	CHECK(b->width == 30);
	CHECK(b->height == 40);
	CHECK(b->accent == 0x801F);
	CHECK(findThumb(entries, thumbKey("missing.gb")) == nullptr);

	std::vector<uint8_t> bad = file;
	bad[4] = 2; // unknown version
	CHECK_FALSE(parseThumbHeader(bad.data(), bad.size(), count));
	bad = file;
	bad[kThumbHeaderSize + 8] = kThumbMaxSize + 1; // too wide
	CHECK_FALSE(parseThumbEntries(bad.data() + kThumbHeaderSize, bad.size() - kThumbHeaderSize, 3, entries));
}

TEST_CASE("ThumbCache loads each thumbnail once and remembers failures") {
	const std::vector<uint8_t> file = thumbsFile({"a.gb", "b.gb"}, 4, 2);
	uint32_t count = 0;
	parseThumbHeader(file.data(), file.size(), count);
	std::vector<ThumbEntry> entries;
	parseThumbEntries(file.data() + kThumbHeaderSize, file.size() - kThumbHeaderSize, count, entries);

	int loads = 0;
	bool failNext = false;
	ThumbCache cache;
	CHECK(cache.empty());
	cache.setSource(entries, [&](const ThumbEntry& e, uint16_t* out) {
		++loads;
		if (failNext) return false;
		std::memcpy(out, file.data() + e.offset, size_t(e.width) * e.height * 2);
		return true;
	});
	int w = 0, h = 0;
	const uint16_t* a = cache.get("sd:/roms/GB/a.gb", w, h);
	REQUIRE(a != nullptr);
	CHECK(w == 4);
	CHECK(h == 2);
	CHECK((a[0] & 0x8000) != 0);
	cache.get("sd:/roms/GB/a.gb", w, h);
	CHECK(loads == 1);
	CHECK(cache.get("sd:/roms/GB/none.gb", w, h) == nullptr);
	CHECK(loads == 1);
	CHECK(cache.accent("sd:/roms/GB/a.gb") == 0x801F); // from the index, no read
	CHECK(cache.accent("sd:/roms/GB/none.gb") == 0);
	CHECK(loads == 1);

	failNext = true;
	CHECK(cache.get("sd:/roms/GB/b.gb", w, h) == nullptr);
	CHECK(cache.get("sd:/roms/GB/b.gb", w, h) == nullptr);
	CHECK(loads == 2); // not retried
}
