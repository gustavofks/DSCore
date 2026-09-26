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
