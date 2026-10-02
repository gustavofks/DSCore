#include "ui/Backdrop.h"

#include <algorithm>

#include "gfx/Canvas.h"
#include "gfx/Hot.h"

namespace dscore {

namespace {

constexpr int kTiny = 12; // longer side of the shrunk cover; smaller means blurrier
constexpr int kBayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

// Source index and weight (0..256 for the second sample) of each output column or row.
struct Sample {
	uint8_t i0, i1;
	uint16_t weight;
};

void samples(int outSize, int srcSize, Sample* out) {
	for (int o = 0; o < outSize; ++o) {
		// Centre of the output pixel in source pixels, in 1/256 units, minus half a pixel.
		const int pos = ((2 * o + 1) * srcSize * 128) / outSize - 128;
		const int i0 = std::clamp(pos >> 8, 0, srcSize - 1);
		out[o] = {uint8_t(i0), uint8_t(std::min(i0 + 1, srcSize - 1)), uint16_t(std::clamp(pos - i0 * 256, 0, 256))};
	}
}

} // namespace

DSCORE_HOT void buildBackdrop(const Cover& cover, uint16_t base, uint16_t* out) {
	const int w = cover.width, h = cover.height;
	const int tw = w >= h ? kTiny : std::max(1, kTiny * w / h);
	const int th = h >= w ? kTiny : std::max(1, kTiny * h / w);

	// Box-average the cover down to tw x th, one 5-bit value per channel.
	uint8_t tiny[3][kTiny * kTiny];
	for (int y = 0; y < th; ++y) {
		const int y0 = y * h / th, y1 = std::max(y0 + 1, (y + 1) * h / th);
		for (int x = 0; x < tw; ++x) {
			const int x0 = x * w / tw, x1 = std::max(x0 + 1, (x + 1) * w / tw);
			int sum[3] = {0, 0, 0}, n = 0;
			for (int yy = y0; yy < y1; ++yy) {
				for (int xx = x0; xx < x1; ++xx) {
					const uint16_t p = cover.pixels[size_t(yy * w + xx)];
					sum[0] += p & 31;
					sum[1] += (p >> 5) & 31;
					sum[2] += (p >> 10) & 31;
					++n;
				}
			}
			for (int c = 0; c < 3; ++c) tiny[c][y * tw + x] = uint8_t(sum[c] / n);
		}
	}

	static Sample cols[kBackdropW], rows[kBackdropH]; // static: the DS stack (DTCM) is small
	samples(kBackdropW, tw, cols);
	samples(kBackdropH, th, rows);
	const int baseC[3] = {base & 31, (base >> 5) & 31, (base >> 10) & 31};

	for (int y = 0; y < kBackdropH; ++y) {
		const Sample& r = rows[y];
		const int dark = 160 + 80 * y / kBackdropH; // share of base, in 1/256: 10/16 at the top to 15/16
		for (int x = 0; x < kBackdropW; ++x) {
			const Sample& c = cols[x];
			int value[3];
			for (int ch = 0; ch < 3; ++ch) {
				const uint8_t* t = tiny[ch];
				const int top = t[r.i0 * tw + c.i0] * (256 - c.weight) + t[r.i0 * tw + c.i1] * c.weight;
				const int bottom = t[r.i1 * tw + c.i0] * (256 - c.weight) + t[r.i1 * tw + c.i1] * c.weight;
				const int blur = top * (256 - r.weight) + bottom * r.weight; // in 1/65536
				const int mixed = (blur * (256 - dark) + (baseC[ch] << 16) * dark) >> 8;
				value[ch] = std::min(31, ((mixed >> 12) + kBayer[y & 3][x & 3]) >> 4); // dither the 1/16 remainder
			}
			out[y * kBackdropW + x] = rgb(value[0], value[1], value[2]);
		}
	}
}

} // namespace dscore
