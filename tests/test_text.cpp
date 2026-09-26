#include "doctest.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

#include "core/Text.h"

using namespace dscore;

static std::vector<uint8_t> utf16le(std::initializer_list<uint16_t> units) {
	std::vector<uint8_t> out;
	for (uint16_t u : units) {
		out.push_back(uint8_t(u & 0xFF));
		out.push_back(uint8_t(u >> 8));
	}
	return out;
}

TEST_CASE("utf16leToUtf8 converts ASCII and stops at NUL") {
	auto d = utf16le({'D', 'S', 0, 'X'});
	CHECK(utf16leToUtf8(d.data(), 4) == "DS");
}

TEST_CASE("utf16leToUtf8 respects maxUnits") {
	auto d = utf16le({'A', 'B', 'C'});
	CHECK(utf16leToUtf8(d.data(), 2) == "AB");
}

TEST_CASE("utf16leToUtf8 encodes 2- and 3-byte sequences") {
	auto d = utf16le({0x00E9, 0x30DE});
	CHECK(utf16leToUtf8(d.data(), 2) == "\xC3\xA9\xE3\x83\x9E");
}

TEST_CASE("utf16leToUtf8 decodes surrogate pairs") {
	auto d = utf16le({0xD83D, 0xDE00});
	CHECK(utf16leToUtf8(d.data(), 2) == "\xF0\x9F\x98\x80");
}

TEST_CASE("utf16leToUtf8 replaces lone surrogates") {
	auto d = utf16le({0xDE00, 'A'});
	CHECK(utf16leToUtf8(d.data(), 2) == "\xEF\xBF\xBD" "A");
}

TEST_CASE("hasExtension is case-insensitive and needs a name before the extension") {
	CHECK(hasExtension("Game.nds", ".nds"));
	CHECK(hasExtension("GAME.NDS", ".nds"));
	CHECK(hasExtension("Alien Hominid # GBA.GBA", ".gba"));
	CHECK_FALSE(hasExtension("Game.sav", ".nds"));
	CHECK_FALSE(hasExtension(".nds", ".nds"));
	CHECK_FALSE(hasExtension("nds", ".nds"));
}

TEST_CASE("asciiForConsole replaces each non-ASCII character with one '?'") {
	CHECK(asciiForConsole("Pok\xC3\xA9mon") == "Pok?mon");
	CHECK(asciiForConsole("\xE3\x83\x9E\xE3\x83\xAA") == "??");
	CHECK(asciiForConsole("Mario Kart DS") == "Mario Kart DS");
}
