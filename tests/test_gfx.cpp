#include "doctest.h"

#include <string>
#include <vector>

#include "gfx/Canvas.h"
#include "gfx/Font.h"

using namespace dscore;

namespace {

struct TestCanvas {
	std::vector<uint16_t> pixels;
	Canvas canvas;
	TestCanvas(int w, int h) : pixels(size_t(w * h), 0), canvas(pixels.data(), w, h) {}
};

} // namespace

TEST_CASE("rgb sets the visibility bit") {
	CHECK(rgb(31, 0, 0) == 0x801F);
	CHECK(rgb(0, 0, 31) == 0xFC00);
}

TEST_CASE("fillRect clips to the canvas") {
	TestCanvas t(4, 3);
	t.canvas.fillRect({-2, 1, 4, 10}, 7);
	CHECK(t.canvas.pixel(0, 0) == 0);
	CHECK(t.canvas.pixel(0, 1) == 7);
	CHECK(t.canvas.pixel(1, 2) == 7);
	CHECK(t.canvas.pixel(2, 1) == 0);
}

TEST_CASE("strokeRect draws only the border") {
	TestCanvas t(5, 5);
	t.canvas.strokeRect({0, 0, 5, 5}, 9, 1);
	CHECK(t.canvas.pixel(0, 0) == 9);
	CHECK(t.canvas.pixel(4, 2) == 9);
	CHECK(t.canvas.pixel(2, 2) == 0);
}

TEST_CASE("fillRounded leaves the corners and fills the middle") {
	TestCanvas t(20, 12);
	t.canvas.fillRounded({0, 0, 20, 12}, 4, 5);
	CHECK(t.canvas.pixel(0, 0) == 0);   // corner cut
	CHECK(t.canvas.pixel(19, 11) == 0);
	CHECK(t.canvas.pixel(10, 0) == 5);  // straight edge
	CHECK(t.canvas.pixel(0, 6) == 5);
	CHECK(t.canvas.pixel(10, 6) == 5);
}

TEST_CASE("roundCorners paints only the corner pixels") {
	TestCanvas t(10, 10);
	t.canvas.fillRect({0, 0, 10, 10}, 3);
	t.canvas.roundCorners({0, 0, 10, 10}, 8);
	CHECK(t.canvas.pixel(0, 0) == 8);
	CHECK(t.canvas.pixel(2, 0) == 8);
	CHECK(t.canvas.pixel(3, 0) == 3);
	CHECK(t.canvas.pixel(9, 9) == 8);
	CHECK(t.canvas.pixel(5, 5) == 3);
}

TEST_CASE("blendRect mixes pixels towards a color") {
	TestCanvas t(4, 4);
	t.canvas.fillRect({0, 0, 4, 4}, rgb(16, 16, 16));
	t.canvas.blendRect({0, 0, 2, 4}, rgb(0, 0, 0), 8);
	CHECK(t.canvas.pixel(0, 0) == rgb(8, 8, 8));
	CHECK(t.canvas.pixel(3, 0) == rgb(16, 16, 16));
}

TEST_CASE("blit skips transparent pixels and scales") {
	TestCanvas t(4, 4);
	const uint16_t image[4] = {0x801F, 0x0000, 0x0000, 0x83E0}; // 2x2: red, clear / clear, green
	t.canvas.fill(5);
	t.canvas.blit(image, 2, 2, 0, 0, 2);
	CHECK(t.canvas.pixel(0, 0) == 0x801F);
	CHECK(t.canvas.pixel(1, 1) == 0x801F);
	CHECK(t.canvas.pixel(2, 0) == 5);
	CHECK(t.canvas.pixel(3, 3) == 0x83E0);
}

TEST_CASE("drawText renders glyph pixels and returns the width") {
	TestCanvas t(20, 16);
	const Font& font = smallFont();
	CHECK(t.canvas.drawText(font, 0, 0, "AA", 1) == 2 * font.width);
	int lit = 0;
	for (int y = 0; y < font.height; ++y) {
		for (int x = 0; x < font.width; ++x) lit += t.canvas.pixel(x, y) == 1;
	}
	CHECK(lit > 10);
	CHECK(t.canvas.pixel(0, 0) == 0); // the top row of 'A' is blank in Spleen
}

TEST_CASE("decodeUtf8 handles multi-byte and malformed input") {
	CHECK(decodeUtf8("A\xC3\xA9") == std::vector<uint32_t>{'A', 0xE9});
	CHECK(decodeUtf8("\xE3\x83\x9E") == std::vector<uint32_t>{0x30DE});
	CHECK(decodeUtf8("\x80" "B") == std::vector<uint32_t>{0xFFFD, 'B'});
	CHECK(decodeUtf8("\xC3") == std::vector<uint32_t>{0xFFFD});
}

TEST_CASE("characters outside Latin-1 use the '?' glyph") {
	const Font& font = smallFont();
	CHECK(font.glyph(0x30DE) == font.glyph('?'));
	CHECK(font.glyph(0xE9) != font.glyph('?'));
}

TEST_CASE("textWidth counts characters, not bytes") {
	CHECK(textWidth(smallFont(), "Pok\xC3\xA9mon") == 7 * smallFont().width);
}

TEST_CASE("ellipsize shortens text that does not fit") {
	const Font& font = smallFont(); // 6 px per character
	CHECK(ellipsize(font, "Metroid", 60) == "Metroid");
	CHECK(ellipsize(font, "Metroid Fusion", 60) == "Metroid...");
	CHECK(ellipsize(font, "Pok\xC3\xA9mon HeartGold", 48) == "Pok\xC3\xA9m...");
}

TEST_CASE("wrapText breaks at spaces and ellipsizes the last line") {
	const Font& font = smallFont(); // 10 characters per 60 px
	CHECK(wrapText(font, "Mario Kart DS", 60, 2) == std::vector<std::string>{"Mario Kart", "DS"});
	CHECK(wrapText(font, "The Legend of Zelda Spirit Tracks", 60, 2) ==
		std::vector<std::string>{"The Legend", "of Zeld..."});
	CHECK(wrapText(font, "Supercalifragilistic", 60, 2) == std::vector<std::string>{"Supercalif", "ragilistic"});
	CHECK(wrapText(font, "Short", 60, 3) == std::vector<std::string>{"Short"});
	CHECK(wrapText(font, "", 60, 2).empty());
}

#include "ui/IconCache.h"

TEST_CASE("IconCache decodes once and evicts the least recently used icon") {
	std::vector<NdsIcon> icons(IconCache::kSlots + 1);
	for (size_t i = 0; i < icons.size(); ++i) {
		icons[i] = NdsIcon{};
		icons[i].bitmap[0] = 0x01;
		icons[i].palette[1] = uint16_t(i);
	}
	IconCache cache;
	const uint16_t* first = cache.get(icons, 0);
	CHECK(first[0] == (0x8000 | 0));
	CHECK(cache.get(icons, 0) == first); // cached: same slot

	icons[0].palette[1] = 0x1234;       // a stale source proves the cached copy is used
	CHECK(cache.get(icons, 0)[0] == (0x8000 | 0));

	for (int i = 1; i <= IconCache::kSlots; ++i) cache.get(icons, i); // fills every slot, evicting 0
	CHECK(cache.get(icons, 0)[0] == (0x8000 | 0x1234));              // decoded again
}

TEST_CASE("Canvas reports the rows drawn since the last call") {
	TestCanvas t(10, 20);
	int first = -1, last = -1;
	REQUIRE(t.canvas.takeDirtyRows(first, last)); // a new canvas is entirely dirty
	CHECK(first == 0);
	CHECK(last == 20);
	CHECK_FALSE(t.canvas.takeDirtyRows(first, last));

	t.canvas.fillRect({2, 5, 3, 4}, 1);
	t.canvas.drawText(smallFont(), 0, 15, "A", 1); // 12 rows tall, clipped at 20
	REQUIRE(t.canvas.takeDirtyRows(first, last));
	CHECK(first == 5);
	CHECK(last == 20);

	t.canvas.fillRect({2, -5, 3, 3}, 1); // fully off-canvas: nothing changes
	CHECK_FALSE(t.canvas.takeDirtyRows(first, last));
}
