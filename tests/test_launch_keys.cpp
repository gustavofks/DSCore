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

namespace {

std::string valueOf(const std::vector<IniKey>& keys, const char* key) {
	for (const IniKey& k : keys) {
		if (k.key == key) return k.value;
	}
	return "<missing>";
}

} // namespace

TEST_CASE("relaunchKeys for a retail DS game") {
	const auto keys = relaunchKeys("sd:/roms/NDS/Mario Kart DS.nds", false);
	CHECK(keys.size() == 5);
	CHECK(valueOf(keys, "ROM_PATH") == "sd:/roms/NDS/Mario Kart DS.nds");
	CHECK(valueOf(keys, "LAUNCH_TYPE") == "1");
	CHECK(valueOf(keys, "PREVIOUS_USED_DEVICE") == "0");
	CHECK(valueOf(keys, "SLOT1_LAUNCHED") == "0");
	CHECK(valueOf(keys, "HOMEBREW_BOOTSTRAP") == "0");
}

TEST_CASE("relaunchKeys marks homebrew DS files for nds-bootstrap-hb") {
	CHECK(valueOf(relaunchKeys("sd:/roms/NDS/Homebrew.nds", true), "HOMEBREW_BOOTSTRAP") == "1");
}

TEST_CASE("relaunchKeys for a GBA game relaunches nds-bootstrap-hb like TWiLight's ROM browser") {
	const auto keys = relaunchKeys("sd:/roms/GBA/Metroid.gba", false);
	CHECK(keys.size() == 6);
	CHECK(valueOf(keys, "ROM_PATH") == "sd:/roms/GBA/Metroid.gba");
	CHECK(valueOf(keys, "LAUNCH_TYPE") == "1");
	CHECK(valueOf(keys, "PREVIOUS_USED_DEVICE") == "0");
	CHECK(valueOf(keys, "SLOT1_LAUNCHED") == "0");
	CHECK(valueOf(keys, "HOMEBREW_BOOTSTRAP") == "1");
	CHECK(valueOf(keys, "HOMEBREW_ARG") == "");
}

TEST_CASE("bootstrapKeys point nds-bootstrap-hb at GBARunner2 with the ROM as argument") {
	const auto keys = bootstrapKeys("sd:/roms/GBA/Sub/Metroid.gba");
	CHECK(valueOf(keys, "NDS_PATH") == "sd:/_nds/GBARunner2_arm7dldi_dsi.nds");
	CHECK(valueOf(keys, "HOMEBREW_ARG") == "fat:/roms/GBA/Sub/Metroid.gba");
	CHECK(valueOf(keys, "RAM_DRIVE_PATH") == "");
	CHECK(valueOf(keys, "DSI_MODE") == "0");
	CHECK(valueOf(keys, "BOOST_CPU") == "1");
	CHECK(valueOf(keys, "BOOST_VRAM") == "0");
}

TEST_CASE("bootstrapKeys is empty for DS games, which TWiLight configures itself") {
	CHECK(bootstrapKeys("sd:/roms/NDS/Game.nds").empty());
}

TEST_CASE("relaunchKeys is empty for unsupported files") {
	CHECK(relaunchKeys("sd:/roms/GBA/Metroid.sav", false).empty());
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
