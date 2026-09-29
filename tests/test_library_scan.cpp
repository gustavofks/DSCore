#include "doctest.h"

#include <optional>
#include <string>
#include <vector>

#include "core/LibraryScan.h"
#include "core/Systems.h"

using namespace dscore;

namespace {

// Parses by file name: .nds files get an icon whose first byte is the path length, except "noicon".
bool fakeParse(const std::string& path, GameEntry& game, std::optional<NdsIcon>& icon) {
	if (path.find("broken") != std::string::npos) return false;
	game.title = path.substr(path.find_last_of('/') + 1);
	systemForPath(path, game.system);
	if (game.system == System::Nds && path.find("noicon") == std::string::npos) {
		icon.emplace();
		icon->bitmap[0] = uint8_t(path.size());
	}
	return true;
}

} // namespace

TEST_CASE("applyScan parses new files and drops missing ones") {
	LibraryData lib;
	std::vector<std::string> parsed;
	const auto parse = [&](const std::string& path, GameEntry& game, std::optional<NdsIcon>& icon) {
		parsed.push_back(path);
		return fakeParse(path, game, icon);
	};

	CHECK(applyScan(lib, {"sd:/roms/NDS/a.nds", "sd:/roms/GBA/b.gba"}, parse));
	CHECK(parsed.size() == 2);
	REQUIRE(lib.games.size() == 2);
	CHECK(lib.icons.size() == 1);

	parsed.clear();
	CHECK(applyScan(lib, {"sd:/roms/NDS/a.nds", "sd:/roms/NDS/c.nds"}, parse));
	CHECK(parsed == std::vector<std::string>{"sd:/roms/NDS/c.nds"});
	REQUIRE(lib.games.size() == 2);
	CHECK(lib.games[0].path == "sd:/roms/NDS/a.nds");
	CHECK(lib.games[1].path == "sd:/roms/NDS/c.nds");
}

TEST_CASE("applyScan reports no change when the file list is the same") {
	LibraryData lib;
	applyScan(lib, {"sd:/roms/NDS/a.nds"}, fakeParse);
	int calls = 0;
	CHECK_FALSE(applyScan(lib, {"sd:/roms/NDS/a.nds"}, [&](const std::string& p, GameEntry& g, std::optional<NdsIcon>& i) {
		++calls;
		return fakeParse(p, g, i);
	}));
	CHECK(calls == 0);
}

TEST_CASE("applyScan compacts icons so indexes stay valid") {
	LibraryData lib;
	applyScan(lib, {"sd:/roms/NDS/a.nds", "sd:/roms/NDS/bb.nds", "sd:/roms/NDS/ccc.nds"}, fakeParse);
	REQUIRE(lib.icons.size() == 3);

	applyScan(lib, {"sd:/roms/NDS/ccc.nds"}, fakeParse);
	REQUIRE(lib.games.size() == 1);
	REQUIRE(lib.icons.size() == 1);
	REQUIRE(lib.games[0].iconIndex == 0);
	CHECK(lib.icons[0].bitmap[0] == std::string("sd:/roms/NDS/ccc.nds").size());
}

TEST_CASE("applyScan skips files it cannot parse and keeps games without icons") {
	LibraryData lib;
	applyScan(lib, {"sd:/roms/NDS/broken.nds", "sd:/roms/GBA/ok.gba", "sd:/roms/NDS/noicon.nds"}, fakeParse);
	REQUIRE(lib.games.size() == 2);
	CHECK(lib.games[0].path == "sd:/roms/GBA/ok.gba");
	CHECK(lib.games[0].iconIndex == -1);
	CHECK(lib.games[1].path == "sd:/roms/NDS/noicon.nds");
	CHECK(lib.games[1].iconIndex == -1);
	CHECK(lib.icons.empty());
}
