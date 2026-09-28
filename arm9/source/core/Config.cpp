#include "core/Config.h"

#include "core/IniText.h"

namespace dscore {

namespace {

constexpr uint32_t kTabCount = 5;
constexpr uint32_t kSortKeyCount = 3;
constexpr uint32_t kViewModeCount = 2;

} // namespace

std::string Config::serialize() const {
	std::string out = "[DSCORE]\n";
	out += "TAB = " + std::to_string(int(tab)) + "\n";
	out += "SORT = " + std::to_string(int(sort)) + "\n";
	out += "VIEW = " + std::to_string(int(view)) + "\n";
	out += "SELECTED = " + selectedPath + "\n";
	out += "THEME = " + theme + "\n";
	out += std::string("SOUND = ") + (sound ? "1" : "0") + "\n";
	return out;
}

Config Config::parse(std::string_view ini) {
	Config config;
	forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view value) {
		if (section != "DSCORE") return;
		uint32_t number = 0;
		const bool isNumber = parseIniUint(value, number);
		if (key == "TAB" && isNumber && number < kTabCount) config.tab = Tab(number);
		else if (key == "SORT" && isNumber && number < kSortKeyCount) config.sort = SortKey(number);
		else if (key == "VIEW" && isNumber && number < kViewModeCount) config.view = ViewMode(number);
		else if (key == "SELECTED") config.selectedPath = std::string(value);
		else if (key == "THEME") config.theme = std::string(value);
		else if (key == "SOUND" && isNumber) config.sound = number != 0;
	});
	return config;
}

} // namespace dscore
