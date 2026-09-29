#include "doctest.h"

#include <string>

#include "core/LaunchKeys.h"

using namespace dscore;

namespace {

bool isSystem(const char* path, System expected) {
	System system;
	return systemForPath(path, system) && system == expected;
}

} // namespace

TEST_CASE("systemForPath uses the extension, ignoring case") {
	CHECK(isSystem("sd:/roms/NDS/a/Game.nds", System::Nds));
	CHECK(isSystem("sd:/roms/NDS/Game.NDS", System::Nds));
	CHECK(isSystem("sd:/roms/GBA/Game.gba", System::Gba));
	CHECK(isSystem("sd:/roms/GBA/Alien Hominid # GBA.GBA", System::Gba));
	CHECK(isSystem("sd:/roms/GB/Tetris.gb", System::Gb));
	CHECK(isSystem("sd:/roms/GB/Pokemon Yellow.SGB", System::Gb));
	CHECK(isSystem("sd:/roms/GBC/Zelda.gbc", System::Gbc));
	CHECK(isSystem("sd:/roms/NES/Metroid.nes", System::Nes));
	CHECK(isSystem("sd:/roms/NES/Zelda.fds", System::Nes));
	CHECK(isSystem("sd:/roms/SMS/Sonic.sms", System::Sms));
	CHECK(isSystem("sd:/roms/GG/Sonic.gg", System::GameGear));
	System system;
	CHECK_FALSE(systemForPath("sd:/roms/GBA/Game.sav", system));
	CHECK(isSystem("sd:/roms/SNES/Super Metroid.sfc", System::Snes));
	CHECK(isSystem("sd:/roms/SNES/Zelda.SMC", System::Snes));
	CHECK(isSystem("sd:/roms/A26/Pitfall!.a26", System::Atari2600));
	CHECK_FALSE(systemForPath("sd:/roms/MD/Sonic.gen", system)); // not launchable yet
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

TEST_CASE("relaunchKeys boots the console's emulator with the ROM as argument") {
	const auto gb = relaunchKeys("sd:/roms/GB/Tetris.gb", false);
	CHECK(gb.size() == 6);
	CHECK(valueOf(gb, "ROM_PATH") == "sd:/roms/GB/Tetris.gb");
	CHECK(valueOf(gb, "LAUNCH_TYPE") == "5");
	CHECK(valueOf(gb, "HOMEBREW_ARG") == "sd:/roms/GB/Tetris.gb");
	CHECK(valueOf(gb, "SLOT1_LAUNCHED") == "0");
	CHECK(valueOf(gb, "PREVIOUS_USED_DEVICE") == "0");
	CHECK(valueOf(relaunchKeys("sd:/roms/GBC/Zelda.gbc", false), "LAUNCH_TYPE") == "5");
	CHECK(valueOf(relaunchKeys("sd:/roms/NES/Metroid.nes", false), "LAUNCH_TYPE") == "4");
	CHECK(valueOf(relaunchKeys("sd:/roms/SMS/Sonic.sms", false), "LAUNCH_TYPE") == "6");
	CHECK(valueOf(relaunchKeys("sd:/roms/GG/Sonic.gg", false), "LAUNCH_TYPE") == "6");
	CHECK(valueOf(relaunchKeys("sd:/roms/A26/Pitfall!.a26", false), "LAUNCH_TYPE") == "9");
	const auto snes = relaunchKeys("sd:/roms/SNES/Super Metroid.sfc", false);
	CHECK(valueOf(snes, "LAUNCH_TYPE") == "21");
	CHECK(valueOf(snes, "HOMEBREW_ARG") == "sd:/roms/SNES/Super Metroid.sfc"); // main.srldr makes it fat:
	CHECK(bootstrapKeys("sd:/roms/GB/Tetris.gb").empty());
}

TEST_CASE("twilightEmulator names the emulator main.srldr boots") {
	CHECK(std::string(twilightEmulator(System::Gbc)) == "sd:/_nds/TWiLightMenu/emulators/gameyob.nds");
	CHECK(std::string(twilightEmulator(System::Nes)) == "sd:/_nds/TWiLightMenu/emulators/nestwl.nds");
	CHECK(std::string(twilightEmulator(System::GameGear)) == "sd:/_nds/TWiLightMenu/emulators/S8DS.nds");
	CHECK(std::string(twilightEmulator(System::Atari2600)) == "sd:/_nds/TWiLightMenu/emulators/StellaDS.nds");
	CHECK(std::string(twilightEmulator(System::Snes)) == "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.srl");
	CHECK(twilightEmulator(System::Nds) == nullptr);
	CHECK(twilightEmulator(System::Gba) == nullptr);
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
