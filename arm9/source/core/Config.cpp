#include "core/Config.h"

#include <iterator>

#include "core/IniText.h"

namespace dscore {

namespace {

constexpr uint32_t kSortKeyCount = 4;
constexpr uint32_t kViewModeCount = 2;

// Tabs of older versions that are now a filter: the index of a fixed tab (0.2) or its id (0.3).
enum class LegacyTab { None, Favorites, Recent };

LegacyTab parseTab(std::string_view value, Tab& tab) {
	uint32_t number = 0;
	if (parseIniUint(value, number)) {
		constexpr Tab kFixed[] = {Tab::all(), Tab::all(), Tab::console(System::Nds), Tab::console(System::Gba), Tab::all()};
		if (number < std::size(kFixed)) tab = kFixed[number];
		return number == 1 ? LegacyTab::Favorites : number == 4 ? LegacyTab::Recent : LegacyTab::None;
	}
	if (value == "favorites" || value == "recent") {
		tab = Tab::all();
		return value == "favorites" ? LegacyTab::Favorites : LegacyTab::Recent;
	}
	tabFromId(value, tab);
	return LegacyTab::None;
}

std::string hiddenIds(uint32_t hidden) {
	std::string out;
	for (int i = 0; i < kSystemCount; ++i) {
		if (!(hidden & (1u << i))) continue;
		if (!out.empty()) out += ',';
		out += systemInfo(System(i)).id;
	}
	return out;
}

uint32_t parseHidden(std::string_view value) {
	uint32_t hidden = 0;
	while (!value.empty()) {
		const size_t comma = value.find(',');
		System system;
		if (systemFromId(trimIni(value.substr(0, comma)), system)) hidden |= 1u << int(system);
		if (comma == std::string_view::npos) break;
		value.remove_prefix(comma + 1);
	}
	return hidden;
}

} // namespace

std::string Config::serialize() const {
	std::string out = "[DSCORE]\n";
	out += "TAB = " + tabId(tab) + "\n";
	out += std::string("FILTER = ") + filterId(filter) + "\n";
	out += "SORT = " + std::to_string(int(sort)) + "\n";
	out += "VIEW = " + std::to_string(int(view)) + "\n";
	out += "SELECTED = " + selectedPath + "\n";
	out += "THEME = " + theme + "\n";
	out += std::string("SOUND = ") + (sound ? "1" : "0") + "\n";
	out += "HIDDEN = " + hiddenIds(hiddenSystems) + "\n";
	return out;
}

Config Config::parse(std::string_view ini) {
	Config config;
	LegacyTab legacy = LegacyTab::None;
	bool hasFilter = false;
	forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view value) {
		if (section != "DSCORE") return;
		uint32_t number = 0;
		const bool isNumber = parseIniUint(value, number);
		if (key == "TAB") legacy = parseTab(value, config.tab);
		else if (key == "FILTER") hasFilter = filterFromId(value, config.filter);
		else if (key == "SORT" && isNumber && number < kSortKeyCount) config.sort = SortKey(number);
		else if (key == "VIEW" && isNumber && number < kViewModeCount) config.view = ViewMode(number);
		else if (key == "SELECTED") config.selectedPath = std::string(value);
		else if (key == "THEME") config.theme = std::string(value);
		else if (key == "SOUND" && isNumber) config.sound = number != 0;
		else if (key == "HIDDEN") config.hiddenSystems = parseHidden(value);
	});
	if (!hasFilter && legacy == LegacyTab::Favorites) config.filter = Filter::Favorites;
	if (!hasFilter && legacy == LegacyTab::Recent) {
		config.filter = Filter::Played;
		config.sort = SortKey::Recent;
	}
	return config;
}

} // namespace dscore
