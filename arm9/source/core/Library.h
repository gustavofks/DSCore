#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/UserData.h"

namespace dscore {

enum class System : uint8_t { Nds = 0, Gba = 1 };

struct GameEntry {
	std::string path;     // e.g. "sd:/roms/NDS/Game.nds"
	std::string title;    // UTF-8 display title
	System system = System::Nds;
	std::string gameCode; // 4 characters, empty when unknown
	uint32_t fileSize = 0;
};

enum class Tab : uint8_t { All, Favorites, Nds, Gba, Recent };
enum class SortKey : uint8_t { Name, System, MostPlayed };

Tab nextTab(Tab tab);
Tab previousTab(Tab tab);
SortKey nextSortKey(SortKey key);
const char* tabLabel(Tab tab);
const char* sortKeyLabel(SortKey key);

// Indexes into games for one tab, in display order. The Recent tab lists played games newest first and
// ignores sort; every other order breaks ties by title, then path.
std::vector<size_t> libraryView(const std::vector<GameEntry>& games, const UserData& data, Tab tab, SortKey sort);

} // namespace dscore
