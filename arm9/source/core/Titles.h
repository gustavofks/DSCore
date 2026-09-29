#pragma once

#include <string>
#include <string_view>

namespace dscore {

// Titles, publishers and tags read from DS banners and ROM file names, cleaned up for DSCore's Latin-1
// fonts.

// Replaces characters the fonts lack but have a close equivalent (dashes, curly quotes, bullets, the
// ellipsis), drops trademark signs and collapses whitespace. Other characters are kept.
std::string cleanForFont(std::string_view text);

// True when the fonts can draw every character of text (Latin-1 only).
bool fitsFont(std::string_view text);

// Display title from a ROM path: the file name without extension, release numbers ("4273 - "), site
// names ("www.example.com") and trailing "(...)" / "[...]" tags, with underscores read as spaces when
// the name has none and a No-Intro trailing article moved to the front ("Legend of Zelda, The" becomes
// "The Legend of Zelda"). Returns the bare name if stripping would leave nothing.
std::string titleFromFileName(std::string_view path);

struct BannerText {
	std::string title;
	std::string publisher; // empty when the banner does not name one
};

// Splits a DS banner title: with two or three lines the last one is the publisher and the others make
// the title. fileTitle (see titleFromFileName) replaces a title the fonts cannot draw, e.g. Japanese, and
// an all-caps title with the same words; other all-caps titles get title case.
BannerText parseBannerText(std::string_view text, std::string_view fileTitle);

// What the tags of a ROM file name say about it.
struct FileTags {
	std::string region;    // e.g. "USA, Europe"; empty when unknown
	std::string languages; // e.g. "En Fr De"; empty when not listed
	bool portuguese = false; // a Portuguese release or fan translation, e.g. "(BR)", "(PT)", "(En,Pt)"
};

// Reads No-Intro and GoodTools style tags ("(USA, Europe) (En,Fr)", "(U) [!]") and fan-translation
// markers, including a parent folder named "br", "pt" or "pt-br".
FileTags parseFileTags(std::string_view path);

} // namespace dscore
