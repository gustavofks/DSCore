#include "doctest.h"

#include <string>

#include "core/LaunchKeys.h"

using namespace dscore;

TEST_CASE("romKindFor uses the extension, ignoring case") {
	CHECK(romKindFor("sd:/roms/NDS/a/Game.nds") == RomKind::Nds);
	CHECK(romKindFor("sd:/roms/NDS/Game.NDS") == RomKind::Nds);
	CHECK(romKindFor("sd:/roms/GBA/Game.gba") == RomKind::Gba);
	CHECK(romKindFor("sd:/roms/GBA/Alien Hominid # GBA.GBA") == RomKind::Gba);
	CHECK(romKindFor("sd:/roms/GBA/Game.sav") == RomKind::Unsupported);
}

TEST_CASE("relaunchKeys for a DS game") {
	const auto keys = relaunchKeys("sd:/roms/NDS/Mario Kart DS.nds");
	REQUIRE(keys.size() == 3);
	CHECK(std::string(keys[0].key) == "ROM_PATH");
	CHECK(keys[0].value == "sd:/roms/NDS/Mario Kart DS.nds");
	CHECK(std::string(keys[1].key) == "LAUNCH_TYPE");
	CHECK(keys[1].value == "1");
	CHECK(std::string(keys[2].key) == "PREVIOUS_USED_DEVICE");
	CHECK(keys[2].value == "0");
}

TEST_CASE("relaunchKeys for a GBA game uses GBARunner2") {
	const auto keys = relaunchKeys("sd:/roms/GBA/Metroid.gba");
	REQUIRE(keys.size() == 3);
	CHECK(keys[1].value == "17");
}

TEST_CASE("relaunchKeys is empty for unsupported files") {
	CHECK(relaunchKeys("sd:/roms/GBA/Metroid.sav").empty());
}

TEST_CASE("usesRsetMarker finds the 'RSET' constant anywhere in the ARM9 binary") {
	const std::string start = std::string("RSET") + std::string(16, '\0');
	const std::string middle = std::string(8, '\x11') + "RSET" + std::string(8, '\x22');
	const std::string end = std::string(16, '\0') + "RSET";
	CHECK(usesRsetMarker(reinterpret_cast<const uint8_t*>(start.data()), start.size()));
	CHECK(usesRsetMarker(reinterpret_cast<const uint8_t*>(middle.data()), middle.size()));
	CHECK(usesRsetMarker(reinterpret_cast<const uint8_t*>(end.data()), end.size()));
}

TEST_CASE("usesRsetMarker is false without the full constant") {
	const std::string partial = std::string(16, '\0') + "RSE";
	const std::string other = "RESET RSEX TESR";
	CHECK_FALSE(usesRsetMarker(reinterpret_cast<const uint8_t*>(partial.data()), partial.size()));
	CHECK_FALSE(usesRsetMarker(reinterpret_cast<const uint8_t*>(other.data()), other.size()));
	CHECK_FALSE(usesRsetMarker(nullptr, 0));
}

TEST_CASE("relaunchMarker matches the protocol of the installed main.srldr") {
	CHECK(relaunchMarker(true) == 0x54455352u);
	CHECK(relaunchMarker(false) == 0u);
}
