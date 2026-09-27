#include "gfx/Canvas.h"

#include <algorithm>

#include "gfx/Hot.h"

namespace dscore {

namespace {

// Word view of the pixel buffer: two pixels per store, which the DS's 32-bit bus handles in one access.
typedef uint32_t __attribute__((may_alias)) PixelPair;

DSCORE_HOT void fillRow(uint16_t* row, int count, uint16_t color) {
	if (count <= 0) return;
	if (reinterpret_cast<uintptr_t>(row) & 2) {
		*row++ = color;
		--count;
	}
	PixelPair* pairs = reinterpret_cast<PixelPair*>(row);
	const PixelPair pair = color | (PixelPair(color) << 16);
	for (int i = 0; i < count / 2; ++i) pairs[i] = pair;
	if (count & 1) row[count - 1] = color;
}

} // namespace

void Canvas::fill(uint16_t color) {
	fillRow(pixels_, width_ * height_, color);
	markDirty(0, height_);
}

void Canvas::fillRect(const Rect& r, uint16_t color) {
	const int x0 = std::max(r.x, 0), y0 = std::max(r.y, 0);
	const int x1 = std::min(r.x + r.w, width_), y1 = std::min(r.y + r.h, height_);
	for (int y = y0; y < y1; ++y) fillRow(pixels_ + y * width_ + x0, x1 - x0, color);
	if (x1 > x0) markDirty(y0, y1);
}

void Canvas::strokeRect(const Rect& r, uint16_t color, int thickness) {
	fillRect({r.x, r.y, r.w, thickness}, color);
	fillRect({r.x, r.y + r.h - thickness, r.w, thickness}, color);
	fillRect({r.x, r.y, thickness, r.h}, color);
	fillRect({r.x + r.w - thickness, r.y, thickness, r.h}, color);
}

DSCORE_HOT void Canvas::blit(const uint16_t* src, int srcW, int srcH, int x, int y, int scale) {
	markDirty(y, y + srcH * scale);
	const bool inside = x >= 0 && y >= 0 && x + srcW * scale <= width_ && y + srcH * scale <= height_;
	for (int sy = 0; sy < srcH; ++sy) {
		const uint16_t* srcRow = src + sy * srcW;
		for (int dy = 0; dy < scale; ++dy) {
			const int py = y + sy * scale + dy;
			if (inside) {
				uint16_t* dst = pixels_ + py * width_ + x;
				for (int sx = 0; sx < srcW; ++sx, dst += scale) {
					const uint16_t color = srcRow[sx];
					if (!(color & 0x8000)) continue;
					for (int dx = 0; dx < scale; ++dx) dst[dx] = color;
				}
			} else {
				for (int sx = 0; sx < srcW; ++sx) {
					const uint16_t color = srcRow[sx];
					if (!(color & 0x8000)) continue;
					for (int dx = 0; dx < scale; ++dx) set(x + sx * scale + dx, py, color);
				}
			}
		}
	}
}

DSCORE_HOT int Canvas::drawText(const Font& font, int x, int y, std::string_view text, uint16_t color, int scale) {
	int penX = x;
	const int glyphW = font.width * scale, glyphH = font.height * scale;
	markDirty(y, y + glyphH);
	for (uint32_t cp : decodeUtf8(text)) {
		const uint8_t* rows = font.glyph(cp);
		const bool inside = penX >= 0 && y >= 0 && penX + glyphW <= width_ && y + glyphH <= height_;
		for (int row = 0; row < font.height; ++row) {
			const uint8_t bits = rows[row];
			if (!bits) continue;
			for (int dy = 0; dy < scale; ++dy) {
				const int py = y + row * scale + dy;
				uint16_t* dst = inside ? pixels_ + py * width_ + penX : nullptr;
				for (int col = 0; col < font.width; ++col) {
					if (!(bits & (0x80 >> col))) continue;
					for (int dx = 0; dx < scale; ++dx) {
						if (inside) dst[col * scale + dx] = color;
						else set(penX + col * scale + dx, py, color);
					}
				}
			}
		}
		penX += glyphW;
	}
	return penX - x;
}

bool Canvas::takeDirtyRows(int& first, int& last) {
	if (dirtyFirst_ >= dirtyLast_) return false;
	first = dirtyFirst_;
	last = dirtyLast_;
	dirtyFirst_ = height_;
	dirtyLast_ = 0;
	return true;
}

} // namespace dscore
