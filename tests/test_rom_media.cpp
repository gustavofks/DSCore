#include "doctest.h"

#include <cstring>
#include <string>
#include <vector>

#include "core/RomMedia.h"

using namespace dscore;

TEST_CASE("readNdsIcon copies bitmap and palette from the banner") {
	std::vector<uint8_t> banner(0x240, 0);
	banner[0x20] = 0x21;          // first two pixels: palette 1, 2
	banner[0x220] = 0xFF;         // palette[0] = 0x7FFF (ignored: transparent)
	banner[0x221] = 0x7F;
	banner[0x222] = 0x1F;         // palette[1] = red
	NdsIcon icon;
	REQUIRE(readNdsIcon(banner.data(), banner.size(), icon));
	CHECK(icon.bitmap[0] == 0x21);
	CHECK(icon.palette[1] == 0x001F);
	CHECK_FALSE(readNdsIcon(banner.data(), 0x23F, icon));
}

TEST_CASE("decodeNdsIcon untiles 4bpp pixels and marks transparency") {
	NdsIcon icon{};
	icon.palette[1] = 0x001F; // red
	icon.palette[2] = 0x03E0; // green
	icon.palette[3] = 0x7C00; // blue
	icon.bitmap[0] = 0x21;                 // (0,0)=1, (1,0)=2
	icon.bitmap[32 + 0] = 0x03;            // tile 1 → (8,0)=3
	icon.bitmap[4 * 32 + 4] = 0x10;        // tile 4 row 1 → (0,9)=0, (1,9)=1

	uint16_t pixels[kIconSize * kIconSize];
	decodeNdsIcon(icon, pixels);
	CHECK(pixels[0] == (0x001F | 0x8000));
	CHECK(pixels[1] == (0x03E0 | 0x8000));
	CHECK(pixels[8] == (0x7C00 | 0x8000));
	CHECK(pixels[2] == 0);                 // index 0 is transparent
	CHECK(pixels[9 * kIconSize + 0] == 0);
	CHECK(pixels[9 * kIconSize + 1] == (0x001F | 0x8000));
}

TEST_CASE("parseGbaHeader reads the internal title and game code") {
	std::vector<uint8_t> rom(0xC0, 0);
	std::memcpy(&rom[0xA0], "METROID4USA\0", 12);
	std::memcpy(&rom[0xAC], "AMTE", 4);
	GbaHeaderInfo info;
	REQUIRE(parseGbaHeader(rom.data(), rom.size(), info));
	CHECK(info.title == "METROID4USA");
	CHECK(info.gameCode == "AMTE");
	CHECK_FALSE(parseGbaHeader(rom.data(), 0xAF, info));
}

TEST_CASE("titleFromFileName strips extension and No-Intro tags") {
	CHECK(titleFromFileName("sd:/roms/GBA/Metroid Fusion (USA).gba") == "Metroid Fusion");
	CHECK(titleFromFileName("Classic NES Series - Metroid (USA, Europe).gba") == "Classic NES Series - Metroid");
	CHECK(titleFromFileName("sd:/roms/GBA/Alien Hominid # GBA.GBA") == "Alien Hominid # GBA");
	CHECK(titleFromFileName("4273 - Pokemon Mystery Dungeon (US)(XenoPhobia).nds") == "4273 - Pokemon Mystery Dungeon");
	CHECK(titleFromFileName("(Weird).gba") == "(Weird)");
}

TEST_CASE("initialsFor picks up to two initials from the title") {
	CHECK(initialsFor("Metroid Zero Mission") == "MZ");
	CHECK(initialsFor("Classic NES Series - Metroid") == "CN");
	CHECK(initialsFor("tetris") == "T");
	CHECK(initialsFor("  ") == "?");
	CHECK(initialsFor("4273 - Pokemon") == "4P");
}
