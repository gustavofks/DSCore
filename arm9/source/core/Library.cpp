#include "core/Library.h"

#include <algorithm>

namespace dscore {

namespace {

constexpr int kTabCount = 5;
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
	switch (tab) {
		case Tab::All: return true;
		case Tab::Favorites: return stats.favorite;
		case Tab::Nds: return game.system == System::Nds;
		case Tab::Gba: return game.system == System::Gba;
		case Tab::Recent: return stats.lastPlayed != 0;
	}
	return false;
}

} // namespace

Tab nextTab(Tab tab) {
	return Tab((int(tab) + 1) % kTabCount);
}

Tab previousTab(Tab tab) {
	return Tab((int(tab) + kTabCount - 1) % kTabCount);
}

SortKey nextSortKey(SortKey key) {
	return SortKey((int(key) + 1) % kSortKeyCount);
}

const char* tabLabel(Tab tab) {
	switch (tab) {
		case Tab::All: return "All";
		case Tab::Favorites: return "Favorites";
		case Tab::Nds: return "DS";
		case Tab::Gba: return "GBA";
		case Tab::Recent: return "Recent";
	}
	return "";
}

const char* sortKeyLabel(SortKey key) {
	switch (key) {
		case SortKey::Name: return "Name";
		case SortKey::System: return "System";
		case SortKey::MostPlayed: return "Most played";
	}
	return "";
}

std::vector<size_t> libraryView(const std::vector<GameEntry>& games, const UserData& data, Tab tab, SortKey sort) {
	std::vector<size_t> view;
	std::vector<GameStats> stats(games.size());
	for (size_t i = 0; i < games.size(); ++i) {
		stats[i] = statsFor(data, games[i]);
		if (inTab(games[i], stats[i], tab)) view.push_back(i);
	}

	std::stable_sort(view.begin(), view.end(), [&](size_t a, size_t b) {
		if (tab == Tab::Recent) {
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
