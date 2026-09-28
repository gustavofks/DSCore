#include "core/Search.h"

#include "core/Text.h"

namespace dscore {

namespace {

// Base letter for U+00C0..U+00FF; 0 keeps the character as it is.
constexpr char kLatin1Base[64] = {
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i', // C0-CF
	'd', 'n', 'o', 'o', 'o', 'o', 'o', 0,   'o', 'u', 'u', 'u', 'u', 'y', 0,   's', // D0-DF
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i', // E0-EF
	'd', 'n', 'o', 'o', 'o', 'o', 'o', 0,   'o', 'u', 'u', 'u', 'u', 'y', 0,   'y', // F0-FF
};

} // namespace

std::string foldForSearch(std::string_view text) {
	std::string out;
	for (uint32_t cp : decodeUtf8(text)) {
		if (cp >= 'A' && cp <= 'Z') cp += 'a' - 'A';
		else if (cp >= 0xC0 && cp <= 0xFF && kLatin1Base[cp - 0xC0]) cp = uint32_t(kLatin1Base[cp - 0xC0]);
		appendUtf8(out, cp);
	}
	return out;
}

bool matchesQuery(std::string_view title, std::string_view query) {
	if (query.empty()) return true;
	return foldForSearch(title).find(foldForSearch(query)) != std::string::npos;
}

} // namespace dscore
