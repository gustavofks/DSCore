#include "doctest.h"

#include <cstring>
#include <vector>

#include "core/Cover.h"

using namespace dscore;

namespace {

std::vector<uint8_t> makeCover(int width, int height) {
	std::vector<uint8_t> data = {'D', 'S', 'C', 'V', uint8_t(width), uint8_t(width >> 8), uint8_t(height), uint8_t(height >> 8)};
	for (int i = 0; i < width * height; ++i) {
		const uint16_t color = uint16_t(0x8000 | i);
		data.push_back(uint8_t(color));
		data.push_back(uint8_t(color >> 8));
	}
	return data;
}

} // namespace

TEST_CASE("decodeCover reads size and pixels") {
	const auto data = makeCover(3, 2);
	const auto cover = decodeCover(data.data(), data.size());
	REQUIRE(cover.has_value());
	CHECK(cover->width == 3);
	CHECK(cover->height == 2);
	REQUIRE(cover->pixels.size() == 6);
	CHECK(cover->pixels[0] == 0x8000);
	CHECK(cover->pixels[5] == 0x8005);
}

TEST_CASE("decodeCover rejects bad data") {
	auto data = makeCover(3, 2);
	CHECK_FALSE(decodeCover(data.data(), data.size() - 1).has_value());
	auto badMagic = data;
	badMagic[0] = 'X';
	CHECK_FALSE(decodeCover(badMagic.data(), badMagic.size()).has_value());
	const auto tooBig = makeCover(kMaxCoverSize + 1, 1);
	CHECK_FALSE(decodeCover(tooBig.data(), tooBig.size()).has_value());
	const auto empty = makeCover(0, 5);
	CHECK_FALSE(decodeCover(empty.data(), empty.size()).has_value());
}

TEST_CASE("coverFileName uses the ROM file name") {
	CHECK(coverFileName("sd:/roms/NDS/Sub/Game (USA).nds") == "Game (USA).nds.bin");
	CHECK(coverFileName("Game.gba") == "Game.gba.bin");
}
