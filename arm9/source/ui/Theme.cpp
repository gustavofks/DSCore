#include "ui/Theme.h"

#include "core/IniText.h"

namespace dscore {

namespace {

int hexDigit(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

// Indexed by System; DS and GBA come from the theme.
constexpr uint16_t kConsoleShades[kSystemCount][4] = {
	{}, {},
	{rgb(13, 16, 6), rgb(11, 15, 7), rgb(15, 17, 8), rgb(10, 13, 5)},  // Game Boy: olive
	{rgb(5, 15, 16), rgb(6, 13, 17), rgb(4, 16, 14), rgb(7, 14, 15)},  // Game Boy Color: teal
	{rgb(13, 13, 15), rgb(15, 13, 13), rgb(12, 12, 13), rgb(14, 14, 12)}, // NES: grey
	{rgb(15, 10, 20), rgb(17, 11, 19), rgb(13, 9, 19), rgb(16, 12, 21)}, // SNES: violet
	{rgb(20, 11, 4), rgb(18, 12, 6), rgb(21, 13, 5), rgb(17, 10, 5)},  // Master System: amber
	{rgb(6, 16, 11), rgb(5, 14, 12), rgb(7, 17, 10), rgb(6, 13, 10)},  // Game Gear: green
	{rgb(6, 8, 16), rgb(7, 9, 14), rgb(5, 7, 15), rgb(8, 9, 17)},      // Mega Drive: navy
};

} // namespace

const uint16_t* tileShades(const Theme& theme, System system) {
	if (system == System::Nds) return theme.ndsShades;
	if (system == System::Gba) return theme.gbaShades;
	return kConsoleShades[int(system) < kSystemCount ? int(system) : 0];
}

const std::vector<Theme>& builtInThemes() {
	static const std::vector<Theme> themes = {
		{"Midnight", rgb(2, 3, 5), rgb(5, 6, 9), rgb(8, 10, 14), rgb(29, 29, 30), rgb(16, 17, 20), rgb(9, 19, 31),
			rgb(31, 25, 6), {rgb(21, 7, 8), rgb(18, 9, 5), rgb(22, 10, 13), rgb(16, 6, 10)},
			{rgb(13, 9, 23), rgb(9, 10, 24), rgb(16, 8, 20), rgb(8, 13, 21)}},
		{"Light", rgb(28, 28, 29), rgb(25, 25, 27), rgb(31, 31, 31), rgb(3, 4, 6), rgb(12, 13, 16), rgb(4, 13, 27),
			rgb(26, 18, 0), {rgb(24, 9, 9), rgb(24, 14, 7), rgb(26, 12, 16), rgb(20, 8, 12)},
			{rgb(15, 11, 25), rgb(11, 13, 26), rgb(19, 10, 23), rgb(10, 16, 24)}},
		{"OLED", rgb(0, 0, 0), rgb(3, 3, 3), rgb(6, 6, 7), rgb(30, 30, 30), rgb(14, 14, 15), rgb(31, 11, 5),
			rgb(31, 25, 6), {rgb(20, 5, 5), rgb(17, 8, 3), rgb(21, 8, 11), rgb(14, 4, 8)},
			{rgb(11, 6, 21), rgb(7, 8, 22), rgb(14, 6, 18), rgb(6, 11, 19)}},
		{"Forest", rgb(2, 5, 3), rgb(4, 8, 5), rgb(7, 12, 8), rgb(27, 30, 26), rgb(14, 18, 14), rgb(12, 26, 10),
			rgb(31, 25, 6), {rgb(20, 8, 6), rgb(18, 11, 4), rgb(21, 10, 10), rgb(15, 7, 7)},
			{rgb(10, 12, 20), rgb(8, 14, 18), rgb(13, 10, 19), rgb(7, 15, 15)}},
	};
	return themes;
}

const Theme& findTheme(const std::vector<Theme>& themes, const std::string& name) {
	for (const Theme& theme : themes) {
		if (theme.name == name) return theme;
	}
	return themes.front();
}

bool parseColor(std::string_view text, uint16_t& color) {
	text = trimIni(text);
	if (text.size() != 7 || text[0] != '#') return false;
	int channels[3];
	for (int i = 0; i < 3; ++i) {
		const int hi = hexDigit(text[1 + 2 * i]), lo = hexDigit(text[2 + 2 * i]);
		if (hi < 0 || lo < 0) return false;
		channels[i] = (hi * 16 + lo) >> 3;
	}
	color = rgb(channels[0], channels[1], channels[2]);
	return true;
}

Theme parseTheme(std::string_view ini, const Theme& base, const std::string& fallbackName) {
	Theme theme = base;
	theme.name = fallbackName.empty() ? base.name : fallbackName;
	struct Field {
		const char* key;
		uint16_t Theme::*color;
	};
	static const Field kFields[] = {
		{"background", &Theme::background}, {"surface", &Theme::surface}, {"surface_high", &Theme::surfaceHigh},
		{"text", &Theme::text}, {"muted", &Theme::muted}, {"accent", &Theme::accent}, {"favorite", &Theme::favorite},
	};
	forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view value) {
		if (section == "theme" && key == "name" && !value.empty()) {
			theme.name = std::string(value);
			return;
		}
		if (section != "colors") return;
		for (const Field& field : kFields) {
			uint16_t color;
			if (key == field.key && parseColor(value, color)) theme.*field.color = color;
		}
	});
	return theme;
}

} // namespace dscore
