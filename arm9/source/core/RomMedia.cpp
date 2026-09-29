#include "core/RomMedia.h"

#include <algorithm>
#include <cstring>
#include <vector>

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

// Articles and "of" rarely tell titles apart.
bool isMinorWord(std::string_view word) {
	std::string lower;
	for (char c : word) lower += (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
	return lower == "the" || lower == "a" || lower == "an" || lower == "of";
}

// No-Intro moves a leading article behind the main title ("Legend of Zelda, The - Link's Awakening");
// put it back in front.
std::string articleFirst(const std::string& title) {
	const size_t dash = title.find(" - ");
	const std::string main = title.substr(0, dash);
	for (const char* article : {"The", "A", "An"}) {
		const std::string suffix = std::string(", ") + article;
		if (main.size() > suffix.size() && main.compare(main.size() - suffix.size(), suffix.size(), suffix) == 0) {
			const std::string rest = dash == std::string::npos ? std::string() : title.substr(dash);
			return article + (" " + main.substr(0, main.size() - suffix.size())) + rest;
		}
	}
	return title;
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
	return title.empty() ? trimSpaces(name) : articleFirst(title);
}

std::string initialsFor(std::string_view title) {
	// Collections repeat their prefix ("Classic NES Series - ..."), so a subtitle tells games apart.
	std::string_view part = title;
	const size_t dash = title.rfind(" - ");
	if (dash != std::string_view::npos) {
		const std::string_view subtitle = title.substr(dash + 3);
		for (char c : subtitle) {
			if (isAlnum(c)) {
				part = subtitle;
				break;
			}
		}
	}

	std::vector<std::string_view> words;
	std::vector<std::string_view> significant;
	size_t pos = 0;
	while (pos < part.size()) {
		const size_t end = std::min(part.find(' ', pos), part.size());
		const std::string_view word = part.substr(pos, end - pos);
		if (!word.empty() && isAlnum(word.front())) {
			words.push_back(word);
			if (!isMinorWord(word)) significant.push_back(word);
		}
		pos = end + 1;
	}
	if (significant.empty()) significant = words;

	std::string initials;
	if (significant.size() >= 2) {
		initials += upperAscii(significant[0].front());
		initials += upperAscii(significant[1].front());
	} else if (significant.size() == 1) {
		for (char c : significant[0]) {
			if (isAlnum(c)) initials += upperAscii(c);
			if (initials.size() == 2) break;
		}
	}
	return initials.empty() ? "?" : initials;
}

} // namespace dscore
