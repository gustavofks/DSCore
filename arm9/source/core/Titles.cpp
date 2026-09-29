#include "core/Titles.h"

#include <algorithm>
#include <vector>

#include "core/Search.h"
#include "core/Text.h"

namespace dscore {

namespace {

std::string trim(std::string_view s) {
	while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
	while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
	return std::string(s);
}

std::string lower(std::string_view s) {
	std::string out(s);
	for (char& c : out) {
		if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
	}
	return out;
}

std::vector<std::string> split(std::string_view s, std::string_view separator) {
	std::vector<std::string> parts;
	size_t start = 0;
	while (true) {
		const size_t at = s.find(separator, start);
		parts.push_back(std::string(s.substr(start, at == std::string_view::npos ? std::string_view::npos : at - start)));
		if (at == std::string_view::npos) return parts;
		start = at + separator.size();
	}
}

std::string fileStem(std::string_view path) {
	const size_t slash = path.find_last_of('/');
	std::string_view name = slash == std::string_view::npos ? path : path.substr(slash + 1);
	const size_t dot = name.find_last_of('.');
	if (dot != std::string_view::npos && dot > 0) name = name.substr(0, dot);
	return std::string(name);
}

// Letters and digits only, lowercase and without accents: "MARIO KART DS" and "Mario Kart DS" match.
std::string wordsKey(std::string_view text) {
	std::string key;
	for (char c : foldForSearch(text)) {
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) key += c;
	}
	return key;
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

bool isSiteName(std::string_view part) {
	const std::string l = lower(trim(part));
	if (l.rfind("www.", 0) == 0) return true;
	for (const char* tld : {".com", ".net", ".org", ".com.br"}) {
		const std::string_view t(tld);
		if (l.size() > t.size() && l.compare(l.size() - t.size(), t.size(), t) == 0) return true;
	}
	return false;
}

// Drops trailing "(...)" and "[...]" groups.
std::string stripTrailingTags(std::string text) {
	while (true) {
		text = trim(text);
		if (text.empty()) return text;
		const char close = text.back();
		if (close != ')' && close != ']') return text;
		const size_t open = text.find_last_of(close == ')' ? '(' : '[');
		if (open == std::string::npos) return text;
		text.resize(open);
	}
}

bool isRomanNumeral(std::string_view word) {
	if (word.empty() || word.size() > 4) return false;
	for (char c : word) {
		if (c != 'I' && c != 'V' && c != 'X') return false;
	}
	return true;
}

bool isMinorWord(std::string_view word) {
	for (const char* w : {"of", "the", "and", "at", "in", "on", "a", "an", "to", "for", "vs", "vs."}) {
		if (word == w) return true;
	}
	return false;
}

// Title case for an all-caps title: "CHRONO TRIGGER" becomes "Chrono Trigger". Roman numerals, "DS" and
// words with digits stay as they are.
std::string titleCase(std::string_view text) {
	std::string out;
	const std::vector<std::string> words = split(text, " ");
	for (size_t i = 0; i < words.size(); ++i) {
		const std::string& word = words[i];
		if (i > 0) out += ' ';
		bool hasDigit = false;
		for (char c : word) hasDigit = hasDigit || (c >= '0' && c <= '9');
		if (hasDigit || isRomanNumeral(word) || word == "DS") {
			out += word;
			continue;
		}
		std::string lowered;
		for (uint32_t cp : decodeUtf8(word)) {
			if ((cp >= 'A' && cp <= 'Z') || (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7)) cp += 0x20;
			appendUtf8(lowered, cp);
		}
		if (i > 0 && isMinorWord(lowered)) {
			out += lowered;
			continue;
		}
		// Capitalize the first letter and each letter after a hyphen or a period ("X-Men", "Dr.").
		bool capitalize = true;
		for (uint32_t cp : decodeUtf8(lowered)) {
			const bool letter = (cp >= 'a' && cp <= 'z') || (cp >= 0xE0 && cp <= 0xFE && cp != 0xF7);
			if (capitalize && letter) {
				cp -= 0x20;
				capitalize = false;
			}
			if (cp == '-' || cp == '.') capitalize = true;
			appendUtf8(out, cp);
		}
	}
	return out;
}

bool isUpperLetter(uint32_t cp) {
	return (cp >= 'A' && cp <= 'Z') || (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7);
}

bool isLowerLetter(uint32_t cp) {
	return (cp >= 'a' && cp <= 'z') || (cp >= 0xDF && cp <= 0xFF && cp != 0xF7);
}

// An all-caps word of five letters or more ("RESIDENT"), not a roman numeral. Shorter ones are often
// acronyms ("GTA", "ROTF", "LEGO").
bool isShouted(std::string_view word) {
	int upper = 0;
	for (uint32_t cp : decodeUtf8(word)) {
		if (isLowerLetter(cp)) return false;
		if (isUpperLetter(cp)) ++upper;
	}
	return upper >= 5 && !isRomanNumeral(word);
}

bool hasShoutedWord(std::string_view text) {
	for (const std::string& word : split(text, " ")) {
		if (isShouted(word)) return true;
	}
	return false;
}

// Title case for the all-caps words only; an all-caps title gets title case as a whole.
std::string calmShoutedWords(std::string_view text) {
	bool anyLower = false;
	for (uint32_t cp : decodeUtf8(text)) anyLower = anyLower || isLowerLetter(cp);
	if (!anyLower) return titleCase(text);
	std::string out;
	for (const std::string& word : split(text, " ")) {
		if (!out.empty()) out += ' ';
		out += isShouted(word) ? titleCase(word) : word;
	}
	return out;
}

// "(C) 2007 Midway" and "©SNK PLAYMORE" become "Midway" and "SNK PLAYMORE".
std::string cleanPublisher(std::string_view publisher) {
	std::string p(publisher);
	for (const char* mark : {"\xC2\xA9", "(C)", "(c)"}) {
		for (size_t at = p.find(mark); at != std::string::npos; at = p.find(mark)) p.erase(at, std::string_view(mark).size());
	}
	p = trim(p);
	if (p.size() > 5 && p[4] == ' ' && std::all_of(p.begin(), p.begin() + 4, [](char c) { return c >= '0' && c <= '9'; })) {
		p = trim(p.substr(5));
	}
	return p;
}

bool isLanguageCode(const std::string& item) {
	const std::string l = lower(item);
	for (const char* code : {"en", "fr", "de", "es", "it", "nl", "pt", "sv", "no", "da", "fi", "ja", "zh", "ko", "pl",
	         "ru", "ca", "cs", "el", "hu", "tr"}) {
		if (l == code) return true;
	}
	return false;
}

// Region names as No-Intro writes them, and GoodTools codes mapped to them.
bool regionName(const std::string& item, std::string& out) {
	static const char* const kNames[] = {"USA", "Europe", "Japan", "World", "Brazil", "Asia", "Australia", "Korea",
		"China", "Germany", "France", "Spain", "Italy", "Netherlands", "Sweden", "Canada", "Russia", "Taiwan",
		"Hong Kong", "UK"};
	for (const char* name : kNames) {
		if (item == name) {
			out = item;
			return true;
		}
	}
	struct Code {
		const char* code;
		const char* region;
	};
	static const Code kCodes[] = {{"U", "USA"}, {"US", "USA"}, {"E", "Europe"}, {"EU", "Europe"}, {"EUR", "Europe"},
		{"J", "Japan"}, {"JPN", "Japan"}, {"W", "World"}, {"UE", "USA, Europe"}, {"UA", "USA, Australia"},
		{"JU", "Japan, USA"}, {"JUE", "World"}, {"K", "Korea"}, {"G", "Germany"}, {"F", "France"}, {"S", "Spain"},
		{"I", "Italy"}};
	for (const Code& code : kCodes) {
		if (item == code.code) {
			out = code.region;
			return true;
		}
	}
	return false;
}

bool isPortugueseMarker(const std::string& item) {
	const std::string l = lower(item);
	return l == "br" || l == "pt" || l == "pt-br" || l == "ptbr" || l == "pt br" || l == "pt_br";
}

} // namespace

std::string cleanForFont(std::string_view text) {
	std::string out;
	for (uint32_t cp : decodeUtf8(text)) {
		switch (cp) {
			case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015: case 0x2212: cp = '-'; break;
			case 0x2018: case 0x2019: case 0x201B: case 0x2032: cp = '\''; break;
			case 0x201C: case 0x201D: case 0x2033: cp = '"'; break;
			case 0x2022: case 0x2027: case 0x30FB: cp = 0xB7; break; // bullets become the middle dot
			case 0x2026: out += "..."; continue;
			case 0x2122: case 0x2120: case 0xAE: continue;             // trademark signs
			case '\t': case '\n': case '\r': case 0xA0: case 0x3000: cp = ' '; break;
			default: break;
		}
		if (cp == ' ' && (out.empty() || out.back() == ' ')) continue;
		appendUtf8(out, cp);
	}
	return trim(out);
}

bool fitsFont(std::string_view text) {
	for (uint32_t cp : decodeUtf8(text)) {
		if (cp < 0x20 || cp > 0xFF || (cp >= 0x7F && cp < 0xA0)) return false;
	}
	return true;
}

std::string titleFromFileName(std::string_view path) {
	std::string name = fileStem(path);
	if (name.find(' ') == std::string::npos) {
		for (char& c : name) {
			if (c == '_') c = ' ';
		}
	}
	name = cleanForFont(name);

	// Release numbers from ROM sets: "4273 - Pokemon Mystery Dungeon".
	size_t digits = 0;
	while (digits < name.size() && name[digits] >= '0' && name[digits] <= '9') ++digits;
	if (digits >= 3 && digits <= 5 && name.compare(digits, 3, " - ") == 0) name.erase(0, digits + 3);

	std::string kept;
	for (const std::string& part : split(name, " - ")) {
		if (isSiteName(stripTrailingTags(part))) continue;
		if (!kept.empty()) kept += " - ";
		kept += part;
	}
	std::string title = stripTrailingTags(kept);
	// Scene release names end in a region and a group: "Dragon Ball Kai Ultimate Butouden JPN NDS-BAHAMUT".
	while (true) {
		const size_t space = title.find_last_of(' ');
		if (space == std::string::npos) break;
		const std::string last = title.substr(space + 1);
		const bool region = last == "JPN" || last == "USA" || last == "EUR" || last == "PAL";
		if (!region && last.rfind("NDS-", 0) != 0 && last.rfind("GBA-", 0) != 0) break;
		title.resize(space);
	}
	while (!title.empty() && (title.back() == '-' || title.back() == ' ')) title.pop_back();
	title = trim(title);
	return title.empty() ? trim(name) : articleFirst(title);
}

BannerText parseBannerText(std::string_view text, std::string_view fileTitle) {
	std::vector<std::string> lines;
	for (const std::string& line : split(text, "\n")) {
		std::string cleaned = cleanForFont(line);
		if (!cleaned.empty()) lines.push_back(std::move(cleaned));
	}
	BannerText out;
	if (lines.empty()) {
		out.title = std::string(fileTitle);
		return out;
	}
	if (lines.size() >= 2) {
		out.publisher = lines.back();
		lines.pop_back();
	}
	for (const std::string& line : lines) {
		if (!out.title.empty()) out.title += ' ';
		out.title += line;
	}

	if (!fitsFont(out.title) && !fileTitle.empty()) {
		out.title = std::string(fileTitle);
	} else if (hasShoutedWord(out.title)) {
		// "MARIO KART DS", "RESIDENT EVIL Deadly Silence": the file name usually has the same words in
		// normal case; otherwise only the all-caps words change.
		out.title = !fileTitle.empty() && wordsKey(out.title) == wordsKey(fileTitle) ? std::string(fileTitle)
		                                                                          : calmShoutedWords(out.title);
	}
	out.publisher = cleanPublisher(out.publisher);
	if (!fitsFont(out.publisher)) out.publisher.clear();
	return out;
}

FileTags parseFileTags(std::string_view path) {
	FileTags tags;
	const std::string name = fileStem(path);
	for (size_t open = name.find_first_of("(["); open != std::string::npos; open = name.find_first_of("([", open + 1)) {
		const size_t close = name.find(name[open] == '(' ? ')' : ']', open + 1);
		if (close == std::string::npos) break;
		std::vector<std::string> items;
		for (const std::string& item : split(name.substr(open + 1, close - open - 1), ",")) items.push_back(trim(item));
		if (name[open] == '[') { // GoodTools flags ("[!]", "[S]" for Super Game Boy): only markers count
			for (const std::string& item : items) tags.portuguese = tags.portuguese || isPortugueseMarker(item);
			continue;
		}

		bool allLanguages = true, allRegions = true;
		std::string regions;
		for (const std::string& item : items) {
			if (isPortugueseMarker(item)) tags.portuguese = true;
			allLanguages = allLanguages && isLanguageCode(item);
			std::string region;
			if (regionName(item, region)) {
				if (!regions.empty()) regions += ", ";
				regions += region;
			} else {
				allRegions = false;
			}
		}
		if (allRegions && tags.region.empty()) {
			tags.region = regions;
		} else if (allLanguages && tags.languages.empty() && !(items.size() == 1 && isPortugueseMarker(items[0]))) {
			for (const std::string& item : items) {
				if (!tags.languages.empty()) tags.languages += ' ';
				tags.languages += item;
				if (lower(item) == "pt") tags.portuguese = true;
			}
		}
	}

	if (lower(name).find("portugu") != std::string::npos) tags.portuguese = true;
	const size_t slash = path.find_last_of('/');
	if (slash != std::string_view::npos && slash > 0) {
		const size_t parentStart = path.find_last_of('/', slash - 1);
		const std::string parent(path.substr(parentStart == std::string_view::npos ? 0 : parentStart + 1,
			slash - (parentStart == std::string_view::npos ? 0 : parentStart + 1)));
		if (isPortugueseMarker(parent)) tags.portuguese = true;
	}
	return tags;
}

} // namespace dscore
