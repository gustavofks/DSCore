#pragma once

#include <cstdint>
#include <string_view>

#include "gfx/Font.h"

namespace dscore {

struct Rect {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;

	bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};

// DS direct-color pixel: 5 bits per channel with bit 15 set, which bitmap backgrounds need to show it.
constexpr uint16_t rgb(int r, int g, int b) {
	return uint16_t(0x8000 | ((b & 31) << 10) | ((g & 31) << 5) | (r & 31));
}

// a blended towards b by t sixteenths (t = 0 gives a, 16 gives b).
constexpr uint16_t mixColor(uint16_t a, uint16_t b, int t) {
	return rgb(((a & 31) * (16 - t) + (b & 31) * t) / 16, (((a >> 5) & 31) * (16 - t) + ((b >> 5) & 31) * t) / 16,
		(((a >> 10) & 31) * (16 - t) + ((b >> 10) & 31) * t) / 16);
}

// Near-black or white, whichever reads better on background.
constexpr uint16_t readableOn(uint16_t background) {
	const int luma = 3 * (background & 31) + 6 * ((background >> 5) & 31) + ((background >> 10) & 31); // 0..310
	return luma > 160 ? rgb(2, 2, 3) : rgb(31, 31, 31);
}

// Software drawing into a 16-bit pixel buffer it does not own. Every call clips to the buffer.
class Canvas {
public:
	Canvas(uint16_t* pixels, int width, int height)
		: pixels_(pixels), width_(width), height_(height), dirtyFirst_(0), dirtyLast_(height) {}

	int width() const { return width_; }
	int height() const { return height_; }
	uint16_t pixel(int x, int y) const { return pixels_[y * width_ + x]; }

	void fill(uint16_t color);
	void fillRect(const Rect& r, uint16_t color);
	void strokeRect(const Rect& r, uint16_t color, int thickness);

	// Blends every pixel of r towards color by t sixteenths (see mixColor), e.g. to darken a band.
	void blendRect(const Rect& r, uint16_t color, int t);

	// fillRect with corners rounded to radius pixels.
	void fillRounded(const Rect& r, int radius, uint16_t color);

	// Paints the corners of r with background, so an image drawn in r looks rounded (radius 3).
	void roundCorners(const Rect& r, uint16_t background);

	// Copies an ARGB1555 image, skipping pixels without bit 15, with each pixel scaled to scale x scale.
	void blit(const uint16_t* src, int srcW, int srcH, int x, int y, int scale = 1);

	// Draws text with its top-left corner at (x, y), each font pixel scaled to scale x scale; returns the
	// width drawn.
	int drawText(const Font& font, int x, int y, std::string_view text, uint16_t color, int scale = 1);

	// Rows touched since the last call, as [first, last); false when nothing changed. Lets the caller copy
	// only those rows to video memory.
	bool takeDirtyRows(int& first, int& last);

private:
	void markDirty(int y0, int y1) {
		if (y0 < dirtyFirst_) dirtyFirst_ = y0 < 0 ? 0 : y0;
		if (y1 > dirtyLast_) dirtyLast_ = y1 > height_ ? height_ : y1;
	}

	void set(int x, int y, uint16_t color) {
		if (x >= 0 && y >= 0 && x < width_ && y < height_) pixels_[y * width_ + x] = color;
	}

	uint16_t* pixels_;
	int width_;
	int height_;
	int dirtyFirst_ = 0;
	int dirtyLast_ = 0;
};

} // namespace dscore
