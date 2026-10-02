#include "doctest.h"

#include <string>

#include "ui/Theme.h"

using namespace dscore;

TEST_CASE("built-in themes start with the default and have distinct names") {
	const auto& themes = builtInThemes();
	REQUIRE(themes.size() >= 3);
	CHECK(themes[0].name == "Cover art");
	CHECK(themes[0].fromCover);
	for (size_t i = 1; i < themes.size(); ++i) {
		CHECK(themes[i].name != themes[0].name);
		CHECK_FALSE(themes[i].fromCover);
	}
}

TEST_CASE("coverTheme tints a theme with a cover's accent color") {
	const Theme& base = builtInThemes()[0];
	const uint16_t red = rgb(26, 6, 4) | 0x8000;
	const Theme t = coverTheme(base, red);
	CHECK(t.accent == red);
	CHECK((t.background & 31) < (red & 31));            // a dark tint of the accent
	CHECK((t.background & 31) > ((t.background >> 10) & 31)); // still reddish
	CHECK(t.surface != t.background);
	CHECK(t.favorite == base.favorite);
	CHECK(coverTheme(base, 0).accent == base.accent); // unknown accent keeps the base
}

TEST_CASE("readableOn picks dark text on light colors and white on dark ones") {
	CHECK(readableOn(rgb(31, 31, 31)) == rgb(2, 2, 3));
	CHECK(readableOn(rgb(28, 24, 4)) == rgb(2, 2, 3));
	CHECK(readableOn(rgb(4, 6, 20)) == rgb(31, 31, 31));
	CHECK(mixColor(rgb(0, 0, 0), rgb(16, 16, 16), 8) == rgb(8, 8, 8));
}

TEST_CASE("custom themes never follow covers") {
	CHECK_FALSE(parseTheme("[colors]\naccent = #FF8000\n", builtInThemes()[0], "Mine").fromCover);
}

TEST_CASE("parseColor reads #RRGGBB into a DS color") {
	uint16_t color = 0;
	REQUIRE(parseColor("#FF0000", color));
	CHECK(color == rgb(31, 0, 0));
	REQUIRE(parseColor("  #00ff80 ", color));
	CHECK(color == rgb(0, 31, 16));
	CHECK_FALSE(parseColor("FF0000", color));
	CHECK_FALSE(parseColor("#GG0000", color));
	CHECK_FALSE(parseColor("#FFF", color));
}

TEST_CASE("parseTheme overrides only the colors it sets") {
	const Theme& base = builtInThemes()[0];
	const Theme theme = parseTheme("[theme]\nname = Sunset\n[colors]\naccent = #FF8000\nbackground=#101010\nbogus = #FFFFFF\ntext = nope\n", base);
	CHECK(theme.name == "Sunset");
	CHECK(theme.accent == rgb(31, 16, 0));
	CHECK(theme.background == rgb(2, 2, 2));
	CHECK(theme.text == base.text);
	CHECK(theme.surface == base.surface);
}

TEST_CASE("parseTheme falls back to the given name") {
	const Theme theme = parseTheme("[colors]\n", builtInThemes()[0], "file-name");
	CHECK(theme.name == "file-name");
}

#include <fstream>
#include <sstream>

TEST_CASE("the example theme shipped in themes/ parses") {
	std::ifstream in("../themes/Sunset.ini");
	REQUIRE(in.good());
	std::stringstream text;
	text << in.rdbuf();
	const Theme& base = builtInThemes()[0];
	const Theme theme = parseTheme(text.str(), base, "Sunset");
	CHECK(theme.name == "Sunset");
	CHECK(theme.accent != base.accent);
	CHECK(theme.background != base.background);
}
