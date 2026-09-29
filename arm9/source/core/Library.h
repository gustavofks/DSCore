#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/Systems.h"
#include "core/UserData.h"

namespace dscore {

struct GameEntry {
	std::string path;     // e.g. "sd:/roms/NDS/Game.nds"
	std::string title;    // UTF-8 display title
	System system = System::Nds;
	std::string gameCode; // 4 characters, empty when unknown
	uint32_t fileSize = 0;
	int32_t iconIndex = -1; // index into LibraryData::icons, -1 when the game has no icon
	std::string publisher;  // from the DS banner, empty when unknown
	bool portuguese = false; // Portuguese release or fan translation (see parseFileTags)
};

// A bottom-screen tab: every game, favorites, recently played, or the games of one console.
struct Tab {
	enum class Kind : uint8_t { All, Favorites, Console, Recent };
	Kind kind = Kind::All;
	System system = System::Nds; // only meaningful for Console

	static constexpr Tab all() { return {Kind::All, System::Nds}; }
	static constexpr Tab favorites() { return {Kind::Favorites, System::Nds}; }
	static constexpr Tab recent() { return {Kind::Recent, System::Nds}; }
	static constexpr Tab console(System system) { return {Kind::Console, system}; }

	bool operator==(const Tab& other) const {
		return kind == other.kind && (kind != Kind::Console || system == other.system);
	}
	bool operator!=(const Tab& other) const { return !(*this == other); }
};

enum class SortKey : uint8_t { Name, System, MostPlayed };

// The tabs shown for this library: All, Favorites, one tab per console with at least one game (in
// System order), then Recent.
std::vector<Tab> availableTabs(const std::vector<GameEntry>& games);

// The tab direction steps away from current (+1 next, -1 previous), wrapping around. A current tab that
// is not in tabs counts as the first one.
Tab stepTab(const std::vector<Tab>& tabs, Tab current, int direction);

SortKey nextSortKey(SortKey key);
const char* tabLabel(Tab tab); // short, fits the tab bar
const char* sortKeyLabel(SortKey key);

// Config file key for a tab ("all", "favorites", "recent" or a system id) and back.
std::string tabId(Tab tab);
bool tabFromId(std::string_view id, Tab& out);

// Indexes into games for one tab, in display order, keeping only titles that match query (see
// matchesQuery). The Recent tab lists played games newest first and ignores sort; every other order
// breaks ties by title, then path.
std::vector<size_t> libraryView(const std::vector<GameEntry>& games, const UserData& data, Tab tab, SortKey sort,
	std::string_view query = {});

} // namespace dscore
