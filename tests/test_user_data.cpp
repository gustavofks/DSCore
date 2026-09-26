#include "doctest.h"

#include <string>

#include "core/UserData.h"

using namespace dscore;

TEST_CASE("UserData starts empty and knows nothing about a game") {
	UserData data;
	CHECK(data.empty());
	CHECK(data.find("sd:/roms/NDS/Game.nds") == nullptr);
}

TEST_CASE("recordLaunch counts launches and keeps the latest time") {
	UserData data;
	data.recordLaunch("sd:/roms/NDS/Game.nds", 100);
	data.recordLaunch("sd:/roms/NDS/Game.nds", 250);
	const GameStats* stats = data.find("sd:/roms/NDS/Game.nds");
	REQUIRE(stats != nullptr);
	CHECK(stats->timesPlayed == 2);
	CHECK(stats->lastPlayed == 250);
	CHECK_FALSE(stats->favorite);
}

TEST_CASE("toggleFavorite flips the flag") {
	UserData data;
	data.toggleFavorite("sd:/roms/GBA/Game.gba");
	CHECK(data.find("sd:/roms/GBA/Game.gba")->favorite);
	data.toggleFavorite("sd:/roms/GBA/Game.gba");
	CHECK_FALSE(data.find("sd:/roms/GBA/Game.gba")->favorite);
}

TEST_CASE("serialize and parse round-trip, including paths with brackets") {
	UserData data;
	data.recordLaunch("sd:/roms/NDS/Pokemon [Hack].nds", 1790000000);
	data.toggleFavorite("sd:/roms/NDS/Pokemon [Hack].nds");
	data.toggleFavorite("sd:/roms/GBA/Metroid.gba");

	const UserData copy = UserData::parse(data.serialize());
	const GameStats* hack = copy.find("sd:/roms/NDS/Pokemon [Hack].nds");
	REQUIRE(hack != nullptr);
	CHECK(hack->favorite);
	CHECK(hack->timesPlayed == 1);
	CHECK(hack->lastPlayed == 1790000000u);
	REQUIRE(copy.find("sd:/roms/GBA/Metroid.gba") != nullptr);
	CHECK(copy.find("sd:/roms/GBA/Metroid.gba")->favorite);
}

TEST_CASE("serialize skips games without any data") {
	UserData data;
	data.toggleFavorite("sd:/roms/GBA/Metroid.gba");
	data.toggleFavorite("sd:/roms/GBA/Metroid.gba");
	CHECK(data.serialize().empty());
}

TEST_CASE("parse ignores unknown keys and garbage") {
	const UserData data = UserData::parse(
		"garbage line\n"
		"[sd:/roms/NDS/Game.nds]\r\n"
		"played = 7\r\n"
		"future_key = 1\r\n"
		"last = not-a-number\r\n");
	REQUIRE(data.find("sd:/roms/NDS/Game.nds") != nullptr);
	CHECK(data.find("sd:/roms/NDS/Game.nds")->timesPlayed == 7);
	CHECK(data.find("sd:/roms/NDS/Game.nds")->lastPlayed == 0);
}

TEST_CASE("importTwilightHistory reads TWiLight's recent and play-count files") {
	const std::string recent =
		"[RECENT]\r\n"
		"sd:/ = dumpTool.nds:UNLAUNCH.DSI\r\n"
		"sd:/roms/NDS = Newest.nds:Older.nds\r\n"
		"sd:/roms/GBA = Metroid.gba\r\n";
	const std::string times =
		"[sd:/roms/NDS]\r\n"
		"Older.nds = 104\r\n"
		"Never Recent.nds = 3\r\n";
	UserData data;
	data.importTwilightHistory(recent, times);

	const GameStats* newest = data.find("sd:/roms/NDS/Newest.nds");
	const GameStats* older = data.find("sd:/roms/NDS/Older.nds");
	REQUIRE(newest != nullptr);
	REQUIRE(older != nullptr);
	CHECK(newest->lastPlayed > older->lastPlayed);
	CHECK(older->timesPlayed == 104);
	CHECK(data.find("sd:/roms/NDS/Never Recent.nds")->timesPlayed == 3);
	CHECK(data.find("sd:/roms/NDS/Never Recent.nds")->lastPlayed == 0);
	CHECK(data.find("sd:/roms/GBA/Metroid.gba")->lastPlayed > 0);
	CHECK(data.find("sd:/dumpTool.nds") != nullptr);
}

TEST_CASE("imported recency is always older than a real launch") {
	UserData data;
	data.importTwilightHistory("[RECENT]\nsd:/roms/NDS = A.nds\n", "");
	data.recordLaunch("sd:/roms/NDS/B.nds", 1);
	CHECK(data.find("sd:/roms/NDS/B.nds")->lastPlayed > data.find("sd:/roms/NDS/A.nds")->lastPlayed);
}
