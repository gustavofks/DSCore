#include "core/Library.h"

#include <algorithm>

#include "core/Search.h"

namespace dscore {

namespace {

constexpr SortKey kSortCycle[] = {SortKey::Name, SortKey::Recent, SortKey::MostPlayed, SortKey::System};
constexpr int kSortKeyCount = int(sizeof(kSortCycle) / sizeof(kSortCycle[0]));

// Case-insensitive ASCII ordering; bytes outside ASCII compare as-is.
bool titleLess(const std::string& a, const std::string& b) {
	const size_t n = std::min(a.size(), b.size());
	for (size_t i = 0; i < n; ++i) {
		char ca = a[i], cb = b[i];
		if (ca >= 'A' && ca <= 'Z') ca = char(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z') cb = char(cb - 'A' + 'a');
		if (ca != cb) return static_cast<unsigned char>(ca) < static_cast<unsigned char>(cb);
	}
	return a.size() < b.size();
}

bool byName(const GameEntry& a, const GameEntry& b) {
	if (titleLess(a.title, b.title)) return true;
	if (titleLess(b.title, a.title)) return false;
	return a.path < b.path;
}

GameStats statsFor(const UserData& data, const GameEntry& game) {
	const GameStats* stats = data.find(game.path);
	return stats ? *stats : GameStats{};
}

bool passes(const GameEntry& game, const GameStats& stats, Tab tab, Filter filter) {
	if (tab.kind == Tab::Kind::Console && game.system != tab.system) return false;
	switch (filter) {
		case Filter::All: return true;
		case Filter::Favorites: return stats.favorite;
		case Filter::Played: return stats.timesPlayed != 0 || stats.lastPlayed != 0;
		case Filter::NotPlayed: return stats.timesPlayed == 0 && stats.lastPlayed == 0;
		case Filter::Portuguese: return game.portuguese;
	}
	return false;
}

} // namespace

std::vector<Tab> availableTabs(const std::vector<GameEntry>& games, uint32_t hidden) {
	bool present[kSystemCount] = {};
	for (const GameEntry& game : games) {
		if (int(game.system) < kSystemCount) present[int(game.system)] = true;
	}
	std::vector<Tab> tabs = {Tab::all()};
	std::vector<System> systems;
	for (int i = 0; i < kSystemCount; ++i) {
		if (present[i] && !(hidden & (1u << i))) systems.push_back(System(i));
	}
	std::sort(systems.begin(), systems.end(), [](System a, System b) { return displayOrder(a) < displayOrder(b); });
	for (System system : systems) tabs.push_back(Tab::console(system));
	return tabs;
}

Tab stepTab(const std::vector<Tab>& tabs, Tab current, int direction) {
	if (tabs.empty()) return current;
	const int count = int(tabs.size());
	int index = 0;
	for (int i = 0; i < count; ++i) {
		if (tabs[size_t(i)] == current) index = i;
	}
	return tabs[size_t(((index + direction) % count + count) % count)];
}

Filter stepFilter(Filter filter, int direction) {
	return Filter(((int(filter) + direction) % kFilterCount + kFilterCount) % kFilterCount);
}

SortKey stepSortKey(SortKey key, int direction) {
	int index = 0;
	for (int i = 0; i < kSortKeyCount; ++i) {
		if (kSortCycle[i] == key) index = i;
	}
	return kSortCycle[((index + direction) % kSortKeyCount + kSortKeyCount) % kSortKeyCount];
}

const char* tabLabel(Tab tab) {
	return tab.kind == Tab::Kind::Console ? systemInfo(tab.system).label : "All";
}

const char* filterLabel(Filter filter) {
	switch (filter) {
		case Filter::All: return "All games";
		case Filter::Favorites: return "Favorites";
		case Filter::Played: return "Played";
		case Filter::NotPlayed: return "Not played";
		case Filter::Portuguese: return "Portugu\xC3\xAAs";
	}
	return "";
}

std::string tabId(Tab tab) {
	return tab.kind == Tab::Kind::Console ? systemInfo(tab.system).id : "all";
}

bool tabFromId(std::string_view id, Tab& out) {
	System system;
	if (id == "all") out = Tab::all();
	else if (systemFromId(id, system)) out = Tab::console(system);
	else return false;
	return true;
}

const char* filterId(Filter filter) {
	switch (filter) {
		case Filter::All: return "all";
		case Filter::Favorites: return "favorites";
		case Filter::Played: return "played";
		case Filter::NotPlayed: return "not-played";
		case Filter::Portuguese: return "portuguese";
	}
	return "all";
}

bool filterFromId(std::string_view id, Filter& out) {
	for (int i = 0; i < kFilterCount; ++i) {
		if (id == filterId(Filter(i))) {
			out = Filter(i);
			return true;
		}
	}
	return false;
}

const char* sortKeyLabel(SortKey key) {
	switch (key) {
		case SortKey::Name: return "Name";
		case SortKey::System: return "System";
		case SortKey::MostPlayed: return "Most played";
		case SortKey::Recent: return "Recent";
	}
	return "";
}

std::vector<size_t> libraryView(const std::vector<GameEntry>& games, const UserData& data, Tab tab, Filter filter,
	SortKey sort, std::string_view query) {
	std::vector<size_t> view;
	std::vector<GameStats> stats(games.size());
	const std::string folded = foldForSearch(query);
	for (size_t i = 0; i < games.size(); ++i) {
		stats[i] = statsFor(data, games[i]);
		if (!passes(games[i], stats[i], tab, filter)) continue;
		if (!folded.empty() && foldForSearch(games[i].title).find(folded) == std::string::npos) continue;
		view.push_back(i);
	}

	std::stable_sort(view.begin(), view.end(), [&](size_t a, size_t b) {
		if (sort == SortKey::Recent) {
			if (stats[a].lastPlayed != stats[b].lastPlayed) return stats[a].lastPlayed > stats[b].lastPlayed;
		} else if (sort == SortKey::System) {
			if (games[a].system != games[b].system) return displayOrder(games[a].system) < displayOrder(games[b].system);
		} else if (sort == SortKey::MostPlayed) {
			if (stats[a].timesPlayed != stats[b].timesPlayed) return stats[a].timesPlayed > stats[b].timesPlayed;
		}
		return byName(games[a], games[b]);
	});
	return view;
}

} // namespace dscore
