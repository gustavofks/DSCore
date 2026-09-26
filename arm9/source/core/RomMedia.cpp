#include "core/RomMedia.h"

#include <cstring>

namespace dscore {

namespace {

constexpr size_t kIconBitmapOffset = 0x20;
constexpr size_t kIconPaletteOffset = 0x220;
constexpr size_t kGbaTitleOffset = 0xA0;
constexpr size_t kGbaGameCodeOffset = 0xAC;
constexpr uint16_t kOpaque = 0x8000;

std::string trimSpaces(std::string_view s) {
	while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
	while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
	return std::string(s);
}

bool isAlnum(char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

char upperAscii(char c) {
	return (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
}

} // namespace

bool readNdsIcon(const uint8_t* banner, size_t len, NdsIcon& out) {
	if (len < kNdsIconBannerEnd) return false;
	std::memcpy(out.bitmap, banner + kIconBitmapOffset, sizeof(out.bitmap));
	for (int i = 0; i < 16; ++i) {
		const uint8_t* p = banner + kIconPaletteOffset + 2 * i;
		out.palette[i] = uint16_t(p[0] | (p[1] << 8));
	}
	return true;
}

void decodeNdsIcon(const NdsIcon& icon, uint16_t* pixels) {
	for (int y = 0; y < kIconSize; ++y) {
		for (int x = 0; x < kIconSize; ++x) {
			const int tile = (y / 8) * 4 + (x / 8);
			const uint8_t byte = icon.bitmap[tile * 32 + (y % 8) * 4 + (x % 8) / 2];
			const int index = (x % 2) ? (byte >> 4) : (byte & 0x0F);
			pixels[y * kIconSize + x] = index ? uint16_t((icon.palette[index] & 0x7FFF) | kOpaque) : 0;
		}
	}
}

bool parseGbaHeader(const uint8_t* rom, size_t len, GbaHeaderInfo& out) {
	if (len < kGbaHeaderEnd) return false;
	out.title.assign(reinterpret_cast<const char*>(rom + kGbaTitleOffset), 12);
	out.title.resize(strnlen(out.title.c_str(), 12));
	out.gameCode.assign(reinterpret_cast<const char*>(rom + kGbaGameCodeOffset), 4);
	return true;
}

std::string titleFromFileName(std::string_view path) {
	const size_t slash = path.find_last_of('/');
	std::string_view name = (slash == std::string_view::npos) ? path : path.substr(slash + 1);
	const size_t dot = name.find_last_of('.');
	if (dot != std::string_view::npos && dot > 0) name = name.substr(0, dot);

	std::string_view stripped = name;
	while (true) {
		while (!stripped.empty() && stripped.back() == ' ') stripped.remove_suffix(1);
		if (stripped.empty()) break;
		const char close = stripped.back();
		if (close != ')' && close != ']') break;
		const size_t open = stripped.find_last_of(close == ')' ? '(' : '[');
		if (open == std::string_view::npos) break;
		stripped = stripped.substr(0, open);
	}
	const std::string title = trimSpaces(stripped);
	return title.empty() ? trimSpaces(name) : title;
}

std::string initialsFor(std::string_view title) {
	std::string initials;
	bool wordStart = true;
	for (char c : title) {
		if (c == ' ') {
			wordStart = true;
			continue;
		}
		if (wordStart && isAlnum(c)) {
			initials += upperAscii(c);
			if (initials.size() == 2) break;
		}
		wordStart = false;
	}
	return initials.empty() ? "?" : initials;
}

} // namespace dscore
