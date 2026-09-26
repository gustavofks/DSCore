#include "doctest.h"

#include <string>
#include <vector>

#include "core/LibraryCache.h"

using namespace dscore;

namespace {

LibraryData sampleLibrary() {
	LibraryData lib;
	lib.games.push_back({"sd:/roms/NDS/Pok\xC3\xA9mon.nds", "Pok\xC3\xA9mon HeartGold", System::Nds, "IPKE", 134217728, 0});
	lib.games.push_back({"sd:/roms/GBA/Metroid.gba", "Metroid Fusion", System::Gba, "AMTE", 8388608, -1});
	NdsIcon icon{};
	icon.bitmap[0] = 0x21;
	icon.bitmap[511] = 0x7F;
	icon.palette[1] = 0x001F;
	icon.palette[15] = 0x7FFF;
	lib.icons.push_back(icon);
	return lib;
}

} // namespace

TEST_CASE("library cache round-trips games and icons") {
	const LibraryData original = sampleLibrary();
	const std::vector<uint8_t> bytes = encodeLibrary(original);

	LibraryData decoded;
	REQUIRE(decodeLibrary(bytes.data(), bytes.size(), decoded));
	REQUIRE(decoded.games.size() == 2);
	CHECK(decoded.games[0].path == original.games[0].path);
	CHECK(decoded.games[0].title == original.games[0].title);
	CHECK(decoded.games[0].system == System::Nds);
	CHECK(decoded.games[0].gameCode == "IPKE");
	CHECK(decoded.games[0].fileSize == 134217728u);
	CHECK(decoded.games[0].iconIndex == 0);
	CHECK(decoded.games[1].system == System::Gba);
	CHECK(decoded.games[1].iconIndex == -1);
	REQUIRE(decoded.icons.size() == 1);
	CHECK(decoded.icons[0].bitmap[0] == 0x21);
	CHECK(decoded.icons[0].bitmap[511] == 0x7F);
	CHECK(decoded.icons[0].palette[1] == 0x001F);
	CHECK(decoded.icons[0].palette[15] == 0x7FFF);
}

TEST_CASE("library cache rejects damaged or foreign data") {
	const std::vector<uint8_t> good = encodeLibrary(sampleLibrary());
	LibraryData out;

	std::vector<uint8_t> badMagic = good;
	badMagic[0] = 'X';
	CHECK_FALSE(decodeLibrary(badMagic.data(), badMagic.size(), out));

	std::vector<uint8_t> badVersion = good;
	badVersion[4] = 0xFF;
	CHECK_FALSE(decodeLibrary(badVersion.data(), badVersion.size(), out));

	std::vector<uint8_t> flipped = good;
	flipped.back() ^= 0x01;
	CHECK_FALSE(decodeLibrary(flipped.data(), flipped.size(), out));

	CHECK_FALSE(decodeLibrary(good.data(), good.size() - 1, out));
	CHECK_FALSE(decodeLibrary(good.data(), 3, out));
}

TEST_CASE("library cache rejects icon indexes out of range") {
	LibraryData lib = sampleLibrary();
	lib.games[1].iconIndex = 5;
	const std::vector<uint8_t> bytes = encodeLibrary(lib);
	LibraryData out;
	CHECK_FALSE(decodeLibrary(bytes.data(), bytes.size(), out));
}

TEST_CASE("empty library round-trips") {
	const std::vector<uint8_t> bytes = encodeLibrary(LibraryData{});
	LibraryData out;
	out.games.push_back({});
	REQUIRE(decodeLibrary(bytes.data(), bytes.size(), out));
	CHECK(out.games.empty());
	CHECK(out.icons.empty());
}
