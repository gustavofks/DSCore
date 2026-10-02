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
	CHECK(isSystem("sd:/roms/MD/Sonic.gen", System::MegaDrive));
	CHECK(isSystem("sd:/roms/MD/Sonic the Hedgehog (USA, Europe).md", System::MegaDrive));
	CHECK_FALSE(systemForPath("sd:/roms/MD/Sonic.smd", system)); // interleaved dumps are not supported
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
	// With NEW_SNES_EMU_VER = 1, the ToolchainGenericDS build of SNEmulDS takes the ROM as argument.
	const auto snes = relaunchKeys("sd:/roms/SNES/Super Metroid.sfc", false, 0, true);
	CHECK(valueOf(snes, "LAUNCH_TYPE") == "21");
	CHECK(valueOf(snes, "HOMEBREW_ARG") == "sd:/roms/SNES/Super Metroid.sfc"); // main.srldr makes it fat:
	CHECK(bootstrapKeys("sd:/roms/SNES/Super Metroid.sfc", 0, true).empty());
	CHECK(bootstrapKeys("sd:/roms/GB/Tetris.gb").empty());
}

TEST_CASE("emulatorFor names the emulator each game needs") {
	CHECK(std::string(emulatorFor("sd:/roms/GBC/Zelda.gbc")) == "sd:/_nds/TWiLightMenu/emulators/gameyob.nds");
	CHECK(std::string(emulatorFor("sd:/roms/NES/Metroid.nes")) == "sd:/_nds/TWiLightMenu/emulators/nestwl.nds");
	CHECK(std::string(emulatorFor("sd:/roms/GG/Sonic.gg")) == "sd:/_nds/TWiLightMenu/emulators/S8DS.nds");
	CHECK(std::string(emulatorFor("sd:/roms/A26/Pitfall!.a26")) == "sd:/_nds/TWiLightMenu/emulators/StellaDS.nds");
	CHECK(std::string(emulatorFor("sd:/roms/SNES/Mario.sfc")) == "sd:/_nds/TWiLightMenu/emulators/SNEmulDS-legacy.nds");
	CHECK(std::string(emulatorFor("sd:/roms/SNES/Mario.sfc", 0, true)) == "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.srl");
	CHECK(std::string(emulatorFor("sd:/roms/GBA/Metroid.gba")) == "sd:/_nds/GBARunner2_arm7dldi_dsi.nds");
	CHECK(std::string(emulatorFor("sd:/roms/MD/Sonic.gen", 512 * 1024)) == "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds");
	CHECK(std::string(emulatorFor("sd:/roms/MD/SF2.gen", 5 * 1024 * 1024)) ==
		"sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds");
	CHECK(emulatorFor("sd:/roms/NDS/Game.nds") == nullptr);
}

TEST_CASE(".gen games up to 3 MB run in jEnesisDS from a RAM drive") {
	const auto keys = relaunchKeys("sd:/roms/MD/Sonic.gen", false, kJenesisMaxSize);
	CHECK(valueOf(keys, "LAUNCH_TYPE") == "1");
	CHECK(valueOf(keys, "HOMEBREW_BOOTSTRAP") == "1");
	CHECK(valueOf(keys, "HOMEBREW_ARG") == "");
	CHECK(valueOf(keys, "SHOW_MDGEN") == "<missing>");
	const auto bootstrap = bootstrapKeys("sd:/roms/MD/Sonic.GEN", kJenesisMaxSize);
	CHECK(valueOf(bootstrap, "NDS_PATH") == "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds");
	CHECK(valueOf(bootstrap, "HOMEBREW_ARG") == "fat:/ROM.BIN");
	CHECK(valueOf(bootstrap, "RAM_DRIVE_PATH") == "sd:/roms/MD/Sonic.GEN");
	CHECK(temporaryKeys("sd:/roms/MD/Sonic.gen", kJenesisMaxSize).empty());
}

TEST_CASE("SNES games run in SNEmulDS-legacy from a RAM drive, TWiLight's default") {
	// NEW_SNES_EMU_VER = 0: nds-bootstrap-hb loads the ROM as fat:/ROM.SMC, without the CPU boost.
	const auto keys = relaunchKeys("sd:/roms/SNES/Super Mario Kart (USA).sfc", false, 512 * 1024);
	CHECK(valueOf(keys, "LAUNCH_TYPE") == "1");
	CHECK(valueOf(keys, "HOMEBREW_BOOTSTRAP") == "1");
	CHECK(valueOf(keys, "HOMEBREW_ARG") == "");
	const auto bootstrap = bootstrapKeys("sd:/roms/SNES/Super Mario Kart (USA).sfc", 512 * 1024);
	CHECK(valueOf(bootstrap, "NDS_PATH") == "sd:/_nds/TWiLightMenu/emulators/SNEmulDS-legacy.nds");
	CHECK(valueOf(bootstrap, "HOMEBREW_ARG") == "fat:/ROM.SMC");
	CHECK(valueOf(bootstrap, "RAM_DRIVE_PATH") == "sd:/roms/SNES/Super Mario Kart (USA).sfc");
	CHECK(valueOf(bootstrap, "BOOST_CPU") == "0");
	CHECK(valueOf(bootstrap, "DSI_MODE") == "0");
}

TEST_CASE(".md games run in PicoDriveTWL at any size") {
	// nds-bootstrap-hb only builds jEnesisDS's RAM drive from files named .gen (hb/arm9/source/main.cpp).
	const auto keys = relaunchKeys("sd:/roms/MD/Sonic The Hedgehog (USA, Europe).md", false, 512 * 1024);
	CHECK(valueOf(keys, "LAUNCH_TYPE") == "10");
	CHECK(valueOf(keys, "HOMEBREW_ARG") == "sd:/roms/MD/Sonic The Hedgehog (USA, Europe).md");
	CHECK(valueOf(keys, "SHOW_MDGEN") == "1");
	CHECK(bootstrapKeys("sd:/roms/MD/Sonic The Hedgehog (USA, Europe).md", 512 * 1024).empty());
	CHECK(std::string(emulatorFor("sd:/roms/MD/Sonic.md", 512 * 1024)) ==
		"sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds");
	CHECK(temporaryKeys("sd:/roms/MD/Sonic.md", 512 * 1024) == std::vector<std::string>{"SHOW_MDGEN"});
}

TEST_CASE("larger Mega Drive games run in PicoDriveTWL with SHOW_MDGEN changed for that launch") {
	const uint32_t size = kJenesisMaxSize + 1;
	const auto keys = relaunchKeys("sd:/roms/MD/Super Street Fighter II.gen", false, size);
	CHECK(valueOf(keys, "LAUNCH_TYPE") == "10");
	CHECK(valueOf(keys, "HOMEBREW_ARG") == "sd:/roms/MD/Super Street Fighter II.gen");
	CHECK(valueOf(keys, "SHOW_MDGEN") == "1");
	CHECK(bootstrapKeys("sd:/roms/MD/Super Street Fighter II.gen", size).empty());
	CHECK(temporaryKeys("sd:/roms/MD/Super Street Fighter II.gen", size) == std::vector<std::string>{"SHOW_MDGEN"});
	CHECK(temporaryKeys("sd:/roms/GBA/Metroid.gba", size).empty());
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

TEST_CASE("saveFileCandidates follows nds-bootstrap and emulator conventions") {
	CHECK(saveFileCandidates("sd:/roms/NDS/br/Chrono Trigger (BR).nds") ==
		std::vector<std::string>{"sd:/roms/NDS/br/saves/Chrono Trigger (BR).sav"});
	CHECK(saveFileCandidates("sd:/roms/GBC/Donkey Kong Country (USA).gbc") ==
		std::vector<std::string>{"sd:/roms/GBC/Donkey Kong Country (USA).sav"});
	CHECK(saveFileCandidates("sd:/roms/SNES/Super Metroid.sfc") ==
		std::vector<std::string>{"sd:/roms/SNES/Super Metroid.srm", "sd:/roms/SNES/Super Metroid.sav"});
	CHECK(saveFileCandidates("sd:/roms/GBA/readme.txt").empty());
}
