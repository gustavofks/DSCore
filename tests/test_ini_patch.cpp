#include "doctest.h"

#include <string>
#include <type_traits>
#include <vector>

#include "core/IniPatch.h"

using namespace dscore;

// Keys read from a file are built from temporary strings; a non-owning key once wrote freed memory
// into TWiLight's settings.ini.
static_assert(std::is_same_v<decltype(IniKey::key), std::string>, "IniKey must own its key");

TEST_CASE("IniKey built from temporary strings keeps its key") {
	std::vector<IniKey> keys;
	for (const char* name : {"SHOW_MDGEN", "LAUNCH_TYPE"}) keys.push_back({std::string(name), "3"});
	const std::string filler(256, 'x'); // reuses the memory the temporaries had
	CHECK(patchIni("[SRLOADER]\nSHOW_MDGEN = 1\nLAUNCH_TYPE = 10\n", "SRLOADER", keys) ==
		"[SRLOADER]\nSHOW_MDGEN = 3\nLAUNCH_TYPE = 3\n");
	CHECK(filler.size() == 256);
}

TEST_CASE("patchIni replaces keys in place and keeps everything else") {
	const std::string in =
		"[SRLOADER]\n"
		"ROM_FOLDER = sd:/roms/NDS\n"
		"LAUNCH_TYPE = 1\n"
		"ROM_PATH = sd:/roms/NDS/Old.nds\n"
		"[NDS-BOOTSTRAP]\n"
		"ROM_PATH = untouched\n";
	const std::string out = patchIni(in, "SRLOADER", {{"ROM_PATH", "sd:/roms/GBA/New.gba"}, {"LAUNCH_TYPE", "17"}});
	CHECK(out ==
		"[SRLOADER]\n"
		"ROM_FOLDER = sd:/roms/NDS\n"
		"LAUNCH_TYPE = 17\n"
		"ROM_PATH = sd:/roms/GBA/New.gba\n"
		"[NDS-BOOTSTRAP]\n"
		"ROM_PATH = untouched\n");
}

TEST_CASE("patchIni appends missing keys at the end of the section") {
	const std::string in = "[SRLOADER]\nTHEME = 0\n[NDS-BOOTSTRAP]\nDEBUG = 0\n";
	CHECK(patchIni(in, "SRLOADER", {{"PREVIOUS_USED_DEVICE", "0"}}) ==
		"[SRLOADER]\nTHEME = 0\nPREVIOUS_USED_DEVICE = 0\n[NDS-BOOTSTRAP]\nDEBUG = 0\n");
}

TEST_CASE("patchIni appends the section when it is missing") {
	CHECK(patchIni("[OTHER]\nA = 1\n", "SRLOADER", {{"ROM_PATH", "x"}}) ==
		"[OTHER]\nA = 1\n[SRLOADER]\nROM_PATH = x\n");
	CHECK(patchIni("", "SRLOADER", {{"ROM_PATH", "x"}}) == "[SRLOADER]\nROM_PATH = x\n");
}

TEST_CASE("patchIni keeps CRLF line endings") {
	CHECK(patchIni("[SRLOADER]\r\nROM_PATH = a\r\n", "SRLOADER", {{"ROM_PATH", "b"}}) ==
		"[SRLOADER]\r\nROM_PATH = b\r\n");
}

TEST_CASE("patchIni keeps a missing final newline missing") {
	CHECK(patchIni("[SRLOADER]\nROM_PATH = a", "SRLOADER", {{"ROM_PATH", "b"}}) == "[SRLOADER]\nROM_PATH = b");
}

TEST_CASE("patchIni matches keys after trimming spaces") {
	CHECK(patchIni("[SRLOADER]\n  ROM_PATH=a\n", "SRLOADER", {{"ROM_PATH", "b"}}) == "[SRLOADER]\nROM_PATH = b\n");
}
