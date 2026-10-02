#include "doctest.h"

#include <vector>

#include "gfx/Canvas.h"
#include "ui/Backdrop.h"

using namespace dscore;

namespace {

Cover solid(int w, int h, uint16_t color) {
	Cover c;
	c.width = w;
	c.height = h;
	c.pixels.assign(size_t(w * h), color);
	return c;
}

int red(uint16_t c) { return c & 31; }

} // namespace

TEST_CASE("buildBackdrop fades a cover into the base color, darker at the bottom") {
	std::vector<uint16_t> out(size_t(kBackdropW * kBackdropH));
	buildBackdrop(solid(100, 112, rgb(31, 0, 0)), rgb(0, 0, 0), out.data());
	const uint16_t top = out[size_t(10 * kBackdropW + 128)], bottom = out[size_t(185 * kBackdropW + 128)];
	CHECK(red(top) >= 10);              // 6/16 of full red, give or take the dither
	CHECK(red(top) <= 13);
	CHECK(red(bottom) < red(top));
	CHECK(((top >> 5) & 31) == 0);      // no green or blue appear
	CHECK(((top >> 10) & 31) == 0);
	for (uint16_t p : out) CHECK((p & 0x8000) != 0);
}

TEST_CASE("buildBackdrop handles tiny and wide covers") {
	std::vector<uint16_t> out(size_t(kBackdropW * kBackdropH), 0);
	buildBackdrop(solid(1, 1, rgb(0, 31, 0)), rgb(0, 0, 0), out.data());
	CHECK(((out[0] >> 5) & 31) > 0);
	buildBackdrop(solid(112, 40, rgb(0, 0, 31)), rgb(4, 4, 4), out.data());
	CHECK(((out[size_t(100 * kBackdropW)] >> 10) & 31) > 4);
}
