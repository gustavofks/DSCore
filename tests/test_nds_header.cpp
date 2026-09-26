#include "doctest.h"

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "core/NdsHeader.h"

using namespace dscore;

namespace {

std::vector<uint8_t> makeHeader(const char* gameCode, uint32_t bannerOffset) {
	std::vector<uint8_t> h(kNdsHeaderSize, 0);
	std::memcpy(&h[0x0C], gameCode, 4);
	for (int i = 0; i < 4; ++i) h[0x68 + i] = uint8_t(bannerOffset >> (8 * i));
	return h;
}

std::vector<uint8_t> makeBanner(uint16_t version) {
	std::vector<uint8_t> b(kBannerTitlesEnd, 0);
	b[0] = uint8_t(version & 0xFF);
	b[1] = uint8_t(version >> 8);
	return b;
}

void putTitle(std::vector<uint8_t>& banner, BannerLanguage lang, std::u16string_view text) {
	const size_t offset = 0x240 + size_t(lang) * 0x100;
	for (size_t i = 0; i < text.size(); ++i) {
		banner[offset + 2 * i] = uint8_t(text[i] & 0xFF);
		banner[offset + 2 * i + 1] = uint8_t(text[i] >> 8);
	}
}

} // namespace

TEST_CASE("parseNdsHeader reads game code and banner offset") {
	auto h = makeHeader("ASMA", 0x00A1B200);
	NdsHeaderInfo info;
	REQUIRE(parseNdsHeader(h.data(), h.size(), info));
	CHECK(info.gameCode == "ASMA");
	CHECK(info.bannerOffset == 0x00A1B200u);
}

TEST_CASE("parseNdsHeader reads the ARM9 binary location") {
	auto h = makeHeader("SRLA", 0);
	const uint32_t offset = 0x4000;
	const uint32_t size = 0x7D590;
	for (int i = 0; i < 4; ++i) {
		h[0x20 + i] = uint8_t(offset >> (8 * i));
		h[0x2C + i] = uint8_t(size >> (8 * i));
	}
	NdsHeaderInfo info;
	REQUIRE(parseNdsHeader(h.data(), h.size(), info));
	CHECK(info.arm9Offset == offset);
	CHECK(info.arm9Size == size);
}

namespace {

void putU32(std::vector<uint8_t>& h, size_t offset, uint32_t value) {
	for (int i = 0; i < 4; ++i) h[offset + i] = uint8_t(value >> (8 * i));
}

// Typical retail layout: ARM7 loaded into main RAM.
NdsHeaderInfo retailInfo() {
	auto h = makeHeader("ASME", 0x1000);
	std::memcpy(&h[0x00], "SUPERMARIO64", 12);
	putU32(h, 0x20, 0x4000);
	putU32(h, 0x24, 0x02000800);
	putU32(h, 0x28, 0x02000000);
	putU32(h, 0x34, 0x02380000);
	putU32(h, 0x38, 0x02380000);
	NdsHeaderInfo info;
	parseNdsHeader(h.data(), h.size(), info);
	return info;
}

const uint8_t kRetailStart[16] = {0};
// libnds crt0 entry: mov r0,#0x04000000 / str r0,[r0,#0x208] / mov r0,#0x13 / msr cpsr,r0
const uint8_t kLibndsStart[16] = {0x01, 0x03, 0xA0, 0xE3, 0x08, 0x02, 0x80, 0xE5,
                                  0x13, 0x00, 0xA0, 0xE3, 0x00, 0xF0, 0x29, 0xE1};

} // namespace

TEST_CASE("arm9EntryFileOffset maps the ARM9 entry point to a file offset") {
	CHECK(arm9EntryFileOffset(retailInfo()) == 0x4800u);
}

TEST_CASE("isHomebrew is false for a retail game") {
	CHECK_FALSE(isHomebrew(retailInfo(), kRetailStart));
}

TEST_CASE("isHomebrew detects libnds binaries by their ARM9 entry code") {
	CHECK(isHomebrew(retailInfo(), kLibndsStart));
}

TEST_CASE("isHomebrew detects old homebrew that loads ARM7 into IWRAM") {
	NdsHeaderInfo info = retailInfo();
	info.arm7Entry = 0x037F8000;
	info.arm7Ram = 0x037F8000;
	CHECK(isHomebrew(info, kRetailStart));
}

TEST_CASE("isHomebrew detects the special-cased titles") {
	NdsHeaderInfo info = retailInfo();
	info.gameTitle = "UNLAUNCH.DSI";
	CHECK(isHomebrew(info, kRetailStart));
	info.gameTitle = "NMP4BOOT";
	CHECK(isHomebrew(info, kRetailStart));
}

TEST_CASE("parseNdsHeader rejects short buffers") {
	auto h = makeHeader("ASMA", 0x1000);
	NdsHeaderInfo info;
	CHECK_FALSE(parseNdsHeader(h.data(), kNdsHeaderSize - 1, info));
}

TEST_CASE("bannerTitle returns the first line of the chosen language") {
	auto b = makeBanner(1);
	putTitle(b, BannerLanguage::English, u"Mario Kart DS\nNintendo");
	CHECK(bannerTitle(b.data(), b.size(), BannerLanguage::English) == "Mario Kart DS");
}

TEST_CASE("bannerTitle decodes non-ASCII titles") {
	auto b = makeBanner(1);
	putTitle(b, BannerLanguage::Japanese, u"マリオ\nNintendo");
	CHECK(bannerTitle(b.data(), b.size(), BannerLanguage::Japanese) == "\xE3\x83\x9E\xE3\x83\xAA\xE3\x82\xAA");
}

TEST_CASE("bannerTitle is empty for version 0 or short banners") {
	auto zero = makeBanner(0);
	putTitle(zero, BannerLanguage::English, u"X");
	CHECK(bannerTitle(zero.data(), zero.size(), BannerLanguage::English).empty());

	auto ok = makeBanner(1);
	putTitle(ok, BannerLanguage::English, u"X");
	CHECK(bannerTitle(ok.data(), kBannerTitlesEnd - 1, BannerLanguage::English).empty());
}
