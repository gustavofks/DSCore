#include "gfx/Canvas.h"

#include <algorithm>

namespace dscore {

void Canvas::fill(uint16_t color) {
	std::fill(pixels_, pixels_ + width_ * height_, color);
}

void Canvas::fillRect(const Rect& r, uint16_t color) {
	const int x0 = std::max(r.x, 0), y0 = std::max(r.y, 0);
	const int x1 = std::min(r.x + r.w, width_), y1 = std::min(r.y + r.h, height_);
	for (int y = y0; y < y1; ++y) std::fill(pixels_ + y * width_ + x0, pixels_ + y * width_ + std::max(x0, x1), color);
}

void Canvas::strokeRect(const Rect& r, uint16_t color, int thickness) {
	fillRect({r.x, r.y, r.w, thickness}, color);
	fillRect({r.x, r.y + r.h - thickness, r.w, thickness}, color);
	fillRect({r.x, r.y, thickness, r.h}, color);
	fillRect({r.x + r.w - thickness, r.y, thickness, r.h}, color);
}

void Canvas::blit(const uint16_t* src, int srcW, int srcH, int x, int y, int scale) {
	for (int sy = 0; sy < srcH; ++sy) {
		for (int sx = 0; sx < srcW; ++sx) {
			const uint16_t color = src[sy * srcW + sx];
			if (!(color & 0x8000)) continue;
			for (int dy = 0; dy < scale; ++dy) {
				for (int dx = 0; dx < scale; ++dx) set(x + sx * scale + dx, y + sy * scale + dy, color);
			}
		}
	}
}

int Canvas::drawText(const Font& font, int x, int y, std::string_view text, uint16_t color, int scale) {
	int penX = x;
	for (uint32_t cp : decodeUtf8(text)) {
		const uint8_t* rows = font.glyph(cp);
		for (int row = 0; row < font.height; ++row) {
			for (int col = 0; col < font.width; ++col) {
				if (!(rows[row] & (0x80 >> col))) continue;
				for (int dy = 0; dy < scale; ++dy) {
					for (int dx = 0; dx < scale; ++dx) set(penX + col * scale + dx, y + row * scale + dy, color);
				}
			}
		}
		penX += font.width * scale;
	}
	return penX - x;
}

} // namespace dscore
