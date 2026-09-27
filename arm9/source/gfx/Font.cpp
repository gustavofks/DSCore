#include "gfx/Font.h"

#include <algorithm>

namespace dscore {

namespace {

#include "gfx/fonts/spleen_6x12.inc"
#include "gfx/fonts/spleen_8x16.inc"

constexpr uint32_t kFirst = 0x20;
constexpr uint32_t kLast = 0xFF;
constexpr std::string_view kEllipsis = "...";

const Font kSmall = {kSpleen6x12Width, kSpleen6x12Height, &kSpleen6x12Glyphs[0][0]};
const Font kLarge = {kSpleen8x16Width, kSpleen8x16Height, &kSpleen8x16Glyphs[0][0]};

// Byte offset where each character starts; the last element is text.size().
std::vector<size_t> characterStarts(std::string_view text) {
	std::vector<size_t> starts;
	for (size_t i = 0; i < text.size(); ++i) {
		if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) starts.push_back(i);
	}
	starts.push_back(text.size());
	return starts;
}

size_t characterCount(std::string_view text) {
	return characterStarts(text).size() - 1;
}

// First `count` characters of text.
std::string_view prefix(std::string_view text, size_t count) {
	const std::vector<size_t> starts = characterStarts(text);
	return text.substr(0, starts[std::min(count, starts.size() - 1)]);
}

} // namespace

const uint8_t* Font::glyph(uint32_t codepoint) const {
	if (codepoint < kFirst || codepoint > kLast) codepoint = '?';
	return glyphs + (codepoint - kFirst) * uint32_t(height);
}

const Font& smallFont() { return kSmall; }
const Font& largeFont() { return kLarge; }

int textWidth(const Font& font, std::string_view text) {
	return int(characterCount(text)) * font.width;
}

std::string ellipsize(const Font& font, std::string_view text, int maxWidth) {
	if (textWidth(font, text) <= maxWidth) return std::string(text);
	const int maxChars = maxWidth / font.width - int(kEllipsis.size());
	if (maxChars <= 0) return std::string(kEllipsis.substr(0, size_t(std::max(0, maxWidth / font.width))));
	std::string out(prefix(text, size_t(maxChars)));
	while (!out.empty() && out.back() == ' ') out.pop_back();
	return out + std::string(kEllipsis);
}

std::vector<std::string> wrapText(const Font& font, std::string_view text, int maxWidth, int maxLines) {
	std::vector<std::string> lines;
	const size_t maxChars = size_t(std::max(1, maxWidth / font.width));
	std::string_view rest = text;
	while (!rest.empty() && rest.front() == ' ') rest.remove_prefix(1);

	while (!rest.empty() && int(lines.size()) < maxLines) {
		if (int(lines.size()) == maxLines - 1 || characterCount(rest) <= maxChars) {
			lines.push_back(ellipsize(font, rest, maxWidth));
			break;
		}
		std::string_view line = prefix(rest, maxChars);
		const size_t cut = (rest.size() > line.size() && rest[line.size()] == ' ') ? line.size() : line.find_last_of(' ');
		if (cut != std::string_view::npos && cut > 0) line = rest.substr(0, cut);
		std::string trimmed(line);
		while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
		lines.push_back(trimmed);
		rest.remove_prefix(line.size());
		while (!rest.empty() && rest.front() == ' ') rest.remove_prefix(1);
	}
	return lines;
}

} // namespace dscore
