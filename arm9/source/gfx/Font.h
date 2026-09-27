#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/Text.h"

namespace dscore {

// Fixed-cell bitmap font covering U+0020..U+00FF; other characters render as '?'.
// Each glyph is `height` bytes, one per row, bit 7 = leftmost pixel.
struct Font {
	int width;
	int height;
	const uint8_t* glyphs;

	const uint8_t* glyph(uint32_t codepoint) const;
};

const Font& smallFont(); // Spleen 6x12
const Font& largeFont(); // Spleen 8x16

// Width in pixels of text drawn in font.
int textWidth(const Font& font, std::string_view text);

// Shortens text with a trailing "..." so it fits maxWidth pixels.
std::string ellipsize(const Font& font, std::string_view text, int maxWidth);

// Breaks text at spaces into at most maxLines lines of at most maxWidth pixels; words longer than a
// line are cut, and the last line is ellipsized when text remains.
std::vector<std::string> wrapText(const Font& font, std::string_view text, int maxWidth, int maxLines);

} // namespace dscore
