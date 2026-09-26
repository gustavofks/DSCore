#include "core/Text.h"

namespace dscore {

namespace {

void appendUtf8(std::string& out, uint32_t cp) {
	if (cp < 0x80) {
		out += char(cp);
	} else if (cp < 0x800) {
		out += char(0xC0 | (cp >> 6));
		out += char(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		out += char(0xE0 | (cp >> 12));
		out += char(0x80 | ((cp >> 6) & 0x3F));
		out += char(0x80 | (cp & 0x3F));
	} else {
		out += char(0xF0 | (cp >> 18));
		out += char(0x80 | ((cp >> 12) & 0x3F));
		out += char(0x80 | ((cp >> 6) & 0x3F));
		out += char(0x80 | (cp & 0x3F));
	}
}

uint32_t unitAt(const uint8_t* data, size_t i) {
	return data[2 * i] | (uint32_t(data[2 * i + 1]) << 8);
}

char lowerAscii(char c) {
	return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
}

} // namespace

std::string utf16leToUtf8(const uint8_t* data, size_t maxUnits) {
	std::string out;
	for (size_t i = 0; i < maxUnits; ++i) {
		uint32_t u = unitAt(data, i);
		if (u == 0) break;
		if (u >= 0xD800 && u <= 0xDBFF && i + 1 < maxUnits) {
			const uint32_t lo = unitAt(data, i + 1);
			if (lo >= 0xDC00 && lo <= 0xDFFF) {
				appendUtf8(out, 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00));
				++i;
				continue;
			}
		}
		if (u >= 0xD800 && u <= 0xDFFF) u = 0xFFFD;
		appendUtf8(out, u);
	}
	return out;
}

bool hasExtension(std::string_view name, std::string_view ext) {
	if (name.size() <= ext.size()) return false;
	const size_t offset = name.size() - ext.size();
	for (size_t i = 0; i < ext.size(); ++i) {
		if (lowerAscii(name[offset + i]) != lowerAscii(ext[i])) return false;
	}
	return true;
}

std::string asciiForConsole(std::string_view utf8) {
	std::string out;
	for (unsigned char c : utf8) {
		if (c < 0x80) {
			out += char(c);
		} else if ((c & 0xC0) != 0x80) {
			out += '?'; // lead byte of a multi-byte sequence; continuation bytes are skipped
		}
	}
	return out;
}

} // namespace dscore
