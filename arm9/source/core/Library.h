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

// A bottom-screen tab: every game, or the games of one console.
struct Tab {
	enum class Kind : uint8_t { All, Console };
	Kind kind = Kind::All;
	System system = System::Nds; // only meaningful for Console

	static constexpr Tab all() { return {Kind::All, System::Nds}; }
	static constexpr Tab console(System system) { return {Kind::Console, system}; }

	bool operator==(const Tab& other) const {
		return kind == other.kind && (kind != Kind::Console || system == other.system);
	}
	bool operator!=(const Tab& other) const { return !(*this == other); }
};

// Which games of the tab are listed.
enum class Filter : uint8_t { All, Favorites, Played, NotPlayed, Portuguese };
constexpr int kFilterCount = int(Filter::Portuguese) + 1;

// Values are stored in config.ini: append new ones.
enum class SortKey : uint8_t { Name, System, MostPlayed, Recent };

// The tabs shown for this library: All, then one tab per console with at least one game (in
// displayOrder), leaving out consoles whose bit is set in hidden (bit = System value).
std::vector<Tab> availableTabs(const std::vector<GameEntry>& games, uint32_t hidden = 0);

// The tab direction steps away from current (+1 next, -1 previous), wrapping around. A current tab that
// is not in tabs counts as the first one.
Tab stepTab(const std::vector<Tab>& tabs, Tab current, int direction);

Filter stepFilter(Filter filter, int direction);
SortKey stepSortKey(SortKey key, int direction); // Name, Recent, Most played, System
const char* tabLabel(Tab tab); // short, fits the tab bar
const char* filterLabel(Filter filter);
const char* sortKeyLabel(SortKey key);

// Config file keys ("all" or a system id; "favorites", "played", ...) and back.
std::string tabId(Tab tab);
bool tabFromId(std::string_view id, Tab& out);
const char* filterId(Filter filter);
bool filterFromId(std::string_view id, Filter& out);

// Indexes into games for one tab, in display order, keeping the games that pass filter and whose title
// matches query (see matchesQuery). Recent lists played games newest first, then the others; every order
// breaks ties by title, then path.
std::vector<size_t> libraryView(const std::vector<GameEntry>& games, const UserData& data, Tab tab, Filter filter,
	SortKey sort, std::string_view query = {});

} // namespace dscore
