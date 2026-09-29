#include "doctest.h"

#include <string>
#include <vector>

#include "core/Library.h"

using namespace dscore;

namespace {

std::vector<GameEntry> sampleGames() {
	return {
		{"sd:/roms/NDS/zelda.nds", "Zelda: Spirit Tracks", System::Nds, "BKIE", 0},
		{"sd:/roms/GBA/metroid.gba", "Metroid Fusion", System::Gba, "AMTE", 0},
		{"sd:/roms/NDS/mario.nds", "mario kart DS", System::Nds, "AMCE", 0},
		{"sd:/roms/GBA/advance.gba", "Advance Wars", System::Gba, "AWRE", 0},
	};
}

std::vector<std::string> titles(const std::vector<GameEntry>& games, const std::vector<size_t>& view) {
	std::vector<std::string> out;
	for (size_t i : view) out.push_back(games[i].title);
	return out;
}

} // namespace

TEST_CASE("All tab sorted by name ignores case") {
	const auto games = sampleGames();
	const UserData data;
	CHECK(titles(games, libraryView(games, data, Tab::all(), SortKey::Name)) ==
		std::vector<std::string>{"Advance Wars", "mario kart DS", "Metroid Fusion", "Zelda: Spirit Tracks"});
}

TEST_CASE("System tabs only show their system") {
	const auto games = sampleGames();
	const UserData data;
	CHECK(titles(games, libraryView(games, data, Tab::console(System::Nds), SortKey::Name)) ==
		std::vector<std::string>{"mario kart DS", "Zelda: Spirit Tracks"});
	CHECK(titles(games, libraryView(games, data, Tab::console(System::Gba), SortKey::Name)) ==
		std::vector<std::string>{"Advance Wars", "Metroid Fusion"});
}

TEST_CASE("System sort puts DS before GBA, then name") {
	const auto games = sampleGames();
	const UserData data;
	CHECK(titles(games, libraryView(games, data, Tab::all(), SortKey::System)) ==
		std::vector<std::string>{"mario kart DS", "Zelda: Spirit Tracks", "Advance Wars", "Metroid Fusion"});
}

TEST_CASE("Most played sort uses play counts, then name") {
	const auto games = sampleGames();
	UserData data;
	data.recordLaunch("sd:/roms/GBA/metroid.gba", 10);
	data.recordLaunch("sd:/roms/GBA/metroid.gba", 11);
	data.recordLaunch("sd:/roms/NDS/zelda.nds", 12);
	CHECK(titles(games, libraryView(games, data, Tab::all(), SortKey::MostPlayed)) ==
		std::vector<std::string>{"Metroid Fusion", "Zelda: Spirit Tracks", "Advance Wars", "mario kart DS"});
}

TEST_CASE("Favorites tab only shows favorites") {
	const auto games = sampleGames();
	UserData data;
	data.toggleFavorite("sd:/roms/NDS/zelda.nds");
	data.toggleFavorite("sd:/roms/GBA/advance.gba");
	CHECK(titles(games, libraryView(games, data, Tab::favorites(), SortKey::Name)) ==
		std::vector<std::string>{"Advance Wars", "Zelda: Spirit Tracks"});
}

TEST_CASE("Recent tab lists played games newest first whatever the sort key") {
	const auto games = sampleGames();
	UserData data;
	data.recordLaunch("sd:/roms/NDS/mario.nds", 100);
	data.recordLaunch("sd:/roms/GBA/metroid.gba", 200);
	CHECK(titles(games, libraryView(games, data, Tab::recent(), SortKey::Name)) ==
		std::vector<std::string>{"Metroid Fusion", "mario kart DS"});
}

TEST_CASE("Tabs and sort keys cycle and have labels") {
	const std::vector<Tab> tabs = {Tab::all(), Tab::favorites(), Tab::recent()};
	CHECK(stepTab(tabs, Tab::recent(), 1) == Tab::all());
	CHECK(stepTab(tabs, Tab::all(), -1) == Tab::recent());
	CHECK(stepTab(tabs, Tab::console(System::Gba), 1) == Tab::favorites()); // missing tab counts as the first
	CHECK(nextSortKey(SortKey::MostPlayed) == SortKey::Name);
	CHECK(std::string(tabLabel(Tab::favorites())) == "Fav");
	CHECK(std::string(tabLabel(Tab::console(System::Snes))) == "SNES");
	CHECK(std::string(sortKeyLabel(SortKey::System)) == "System");
}

TEST_CASE("Only consoles with games get a tab, in system order") {
	std::vector<GameEntry> games = {
		{"sd:/roms/SNES/a.sfc", "A", System::Snes, "", 0},
		{"sd:/roms/NDS/b.nds", "B", System::Nds, "", 0},
		{"sd:/roms/SNES/c.sfc", "C", System::Snes, "", 0},
	};
	CHECK(availableTabs(games) == std::vector<Tab>{Tab::all(), Tab::favorites(), Tab::console(System::Nds),
		Tab::console(System::Snes), Tab::recent()});
	CHECK(availableTabs({}) == std::vector<Tab>{Tab::all(), Tab::favorites(), Tab::recent()});
	UserData data;
	CHECK(libraryView(games, data, Tab::console(System::Snes), SortKey::Name).size() == 2);
}

TEST_CASE("Tabs compare by console only for console tabs") {
	CHECK(Tab::console(System::Nds) != Tab::console(System::Gba));
	CHECK(Tab{Tab::Kind::All, System::Gba} == Tab::all());
}

TEST_CASE("Tab ids round-trip") {
	for (const Tab tab : {Tab::all(), Tab::favorites(), Tab::recent(), Tab::console(System::Gba), Tab::console(System::MegaDrive)}) {
		Tab parsed = Tab::favorites();
		REQUIRE(tabFromId(tabId(tab), parsed));
		CHECK(parsed == tab);
	}
	Tab unchanged = Tab::recent();
	CHECK_FALSE(tabFromId("psx", unchanged));
	CHECK(unchanged == Tab::recent());
}
