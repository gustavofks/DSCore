#include "doctest.h"

#include <string>
#include <vector>

#include "core/Config.h"
#include "core/Metadata.h"
#include "ui/App.h"

using namespace dscore;

namespace {

std::vector<GameEntry> sampleGames() {
	return {
		{"sd:/roms/GBA/Metroid Fusion (USA).gba", "Metroid Fusion", System::Gba, "AMTE", 0, -1},
		{"sd:/roms/GBA/RPG/Golden Sun (USA).gba", "Golden Sun", System::Gba, "", 0, -1},
		{"sd:/roms/GB/Pokemon - Yellow (UE) [C][!].gb", "Pokemon - Yellow", System::Gb, "", 0, -1},
		{"sd:/roms/NES/Contra (USA).nes", "Contra", System::Nes, "", 0, -1},
	};
}

const char* kIni =
	"; written by fetch_metadata.py\n"
	"[Metroid Fusion (USA).gba]\n"
	"genre = Action\n"
	"year = 2002\n"
	"developer = Nintendo R&D1\n"
	"players = 1\n"
	"[Golden Sun (USA).gba]\n"
	"genre = RPG\n"
	"year = 2001\n"
	"[Pokemon - Yellow (UE) [C][!].gb]\n"
	"genre = RPG\n"
	"year = 1998\n"
	"players = 2\n"
	"[Not on the card.nes]\n"
	"genre = Puzzle\n"
	"year = abc\n";

} // namespace

TEST_CASE("parseMetadata reads one section per file name, brackets included") {
	const MetadataMap meta = parseMetadata(kIni);
	CHECK(meta.size() == 4);
	const GameMeta& metroid = meta.find("Metroid Fusion (USA).gba")->second;
	CHECK(metroid.genre == "Action");
	CHECK(metroid.year == 2002);
	CHECK(metroid.developer == "Nintendo R&D1");
	CHECK(metroid.players == 1);
	CHECK(meta.find("Pokemon - Yellow (UE) [C][!].gb")->second.players == 2);
	CHECK(meta.find("Not on the card.nes")->second.year == 0); // invalid numbers are ignored
}

TEST_CASE("applyMetadata matches by file name and genresOf lists genres once") {
	std::vector<GameEntry> games = sampleGames();
	games[3].genre = "stale";
	CHECK(applyMetadata(parseMetadata(kIni), games) == 3);
	CHECK(games[1].genre == "RPG"); // the folder does not matter
	CHECK(games[2].year == 1998);
	CHECK(games[3].genre.empty()); // cleared when the game has no entry
	CHECK(genresOf(games) == std::vector<std::string>{"Action", "RPG"});
}

TEST_CASE("libraryView filters by genre and sorts by year") {
	std::vector<GameEntry> games = sampleGames();
	applyMetadata(parseMetadata(kIni), games);
	UserData data;
	std::vector<size_t> rpg = libraryView(games, data, Tab::all(), Filter::All, SortKey::Name, {}, "RPG");
	REQUIRE(rpg.size() == 2);
	CHECK(games[rpg[0]].title == "Golden Sun");

	const std::vector<size_t> byYear = libraryView(games, data, Tab::all(), Filter::All, SortKey::Year);
	std::vector<std::string> titles;
	for (size_t i : byYear) titles.push_back(games[i].title);
	CHECK(titles == std::vector<std::string>{"Pokemon - Yellow", "Golden Sun", "Metroid Fusion", "Contra"});
	CHECK(stepSortKey(SortKey::MostPlayed, 1) == SortKey::Year);
}

TEST_CASE("Config keeps the genre, and App forgets a genre no game has") {
	Config config;
	config.genre = "RPG";
	config.sort = SortKey::Year;
	const Config copy = Config::parse(config.serialize());
	CHECK(copy.genre == "RPG");
	CHECK(copy.sort == SortKey::Year);

	LibraryData lib;
	lib.games = sampleGames();
	applyMetadata(parseMetadata(kIni), lib.games);
	UserData data;
	Config shown = copy;
	App app(lib, data, shown);
	CHECK(shown.genre == "RPG");
	REQUIRE(app.selected() != nullptr);
	CHECK(app.selected()->genre == "RPG");

	LibraryData plain;
	plain.games = sampleGames();
	Config gone = copy;
	App other(plain, data, gone);
	CHECK(gone.genre.empty());
}

TEST_CASE("App options menu cycles the genre") {
	LibraryData lib;
	lib.games = sampleGames();
	applyMetadata(parseMetadata(kIni), lib.games);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::Menu);
	app.handle(Action::Down); // Genre
	app.handle(Action::Right);
	CHECK(config.genre == "Action");
	app.handle(Action::Right);
	CHECK(config.genre == "RPG");
	app.handle(Action::Right);
	CHECK(config.genre.empty()); // back to All
	app.handle(Action::Left);
	CHECK(config.genre == "RPG");
}
