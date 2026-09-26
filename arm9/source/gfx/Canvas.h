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

// Software drawing into a 16-bit pixel buffer it does not own. Every call clips to the buffer.
class Canvas {
public:
	Canvas(uint16_t* pixels, int width, int height) : pixels_(pixels), width_(width), height_(height) {}

	int width() const { return width_; }
	int height() const { return height_; }
	uint16_t pixel(int x, int y) const { return pixels_[y * width_ + x]; }

	void fill(uint16_t color);
	void fillRect(const Rect& r, uint16_t color);
	void strokeRect(const Rect& r, uint16_t color, int thickness);

	// Copies an ARGB1555 image, skipping pixels without bit 15, with each pixel scaled to scale x scale.
	void blit(const uint16_t* src, int srcW, int srcH, int x, int y, int scale = 1);

	// Draws text with its top-left corner at (x, y); returns the width drawn.
	int drawText(const Font& font, int x, int y, std::string_view text, uint16_t color);

private:
	void set(int x, int y, uint16_t color) {
		if (x >= 0 && y >= 0 && x < width_ && y < height_) pixels_[y * width_ + x] = color;
	}

	uint16_t* pixels_;
	int width_;
	int height_;
};

} // namespace dscore
