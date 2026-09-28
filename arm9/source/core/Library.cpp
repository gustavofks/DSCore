#include "core/Library.h"

#include <algorithm>

#include "core/Search.h"

namespace dscore {

namespace {

constexpr int kSortKeyCount = 3;

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

bool inTab(const GameEntry& game, const GameStats& stats, Tab tab) {
	switch (tab.kind) {
		case Tab::Kind::All: return true;
		case Tab::Kind::Favorites: return stats.favorite;
		case Tab::Kind::Console: return game.system == tab.system;
		case Tab::Kind::Recent: return stats.lastPlayed != 0;
	}
	return false;
}

} // namespace

std::vector<Tab> availableTabs(const std::vector<GameEntry>& games) {
	bool present[kSystemCount] = {};
	for (const GameEntry& game : games) {
		if (int(game.system) < kSystemCount) present[int(game.system)] = true;
	}
	std::vector<Tab> tabs = {Tab::all(), Tab::favorites()};
	for (int i = 0; i < kSystemCount; ++i) {
		if (present[i]) tabs.push_back(Tab::console(System(i)));
	}
	tabs.push_back(Tab::recent());
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

SortKey nextSortKey(SortKey key) {
	return SortKey((int(key) + 1) % kSortKeyCount);
}

const char* tabLabel(Tab tab) {
	switch (tab.kind) {
		case Tab::Kind::All: return "All";
		case Tab::Kind::Favorites: return "Fav";
		case Tab::Kind::Console: return systemInfo(tab.system).label;
		case Tab::Kind::Recent: return "Recent";
	}
	return "";
}

std::string tabId(Tab tab) {
	switch (tab.kind) {
		case Tab::Kind::All: return "all";
		case Tab::Kind::Favorites: return "favorites";
		case Tab::Kind::Console: return systemInfo(tab.system).id;
		case Tab::Kind::Recent: return "recent";
	}
	return "all";
}

bool tabFromId(std::string_view id, Tab& out) {
	System system;
	if (id == "all") out = Tab::all();
	else if (id == "favorites") out = Tab::favorites();
	else if (id == "recent") out = Tab::recent();
	else if (systemFromId(id, system)) out = Tab::console(system);
	else return false;
	return true;
}

const char* sortKeyLabel(SortKey key) {
	switch (key) {
		case SortKey::Name: return "Name";
		case SortKey::System: return "System";
		case SortKey::MostPlayed: return "Most played";
	}
	return "";
}

std::vector<size_t> libraryView(const std::vector<GameEntry>& games, const UserData& data, Tab tab, SortKey sort,
	std::string_view query) {
	std::vector<size_t> view;
	std::vector<GameStats> stats(games.size());
	const std::string folded = foldForSearch(query);
	for (size_t i = 0; i < games.size(); ++i) {
		stats[i] = statsFor(data, games[i]);
		if (!inTab(games[i], stats[i], tab)) continue;
		if (!folded.empty() && foldForSearch(games[i].title).find(folded) == std::string::npos) continue;
		view.push_back(i);
	}

	std::stable_sort(view.begin(), view.end(), [&](size_t a, size_t b) {
		if (tab.kind == Tab::Kind::Recent) {
			if (stats[a].lastPlayed != stats[b].lastPlayed) return stats[a].lastPlayed > stats[b].lastPlayed;
		} else if (sort == SortKey::System) {
			if (games[a].system != games[b].system) return games[a].system < games[b].system;
		} else if (sort == SortKey::MostPlayed) {
			if (stats[a].timesPlayed != stats[b].timesPlayed) return stats[a].timesPlayed > stats[b].timesPlayed;
		}
		return byName(games[a], games[b]);
	});
	return view;
}

} // namespace dscore
