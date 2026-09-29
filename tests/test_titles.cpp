#include "doctest.h"

#include <string>

#include "core/Titles.h"

using namespace dscore;

TEST_CASE("titleFromFileName strips extension and No-Intro tags") {
	CHECK(titleFromFileName("sd:/roms/GBA/Metroid Fusion (USA).gba") == "Metroid Fusion");
	CHECK(titleFromFileName("Classic NES Series - Metroid (USA, Europe).gba") == "Classic NES Series - Metroid");
	CHECK(titleFromFileName("sd:/roms/GBA/Alien Hominid # GBA.GBA") == "Alien Hominid # GBA");
	CHECK(titleFromFileName("(Weird).gba") == "(Weird)");
	CHECK(titleFromFileName("Legend of Zelda, The - Link's Awakening (USA, Europe) (Rev 2).gb") ==
		"The Legend of Zelda - Link's Awakening");
	CHECK(titleFromFileName("Legend of Zelda, The (USA) (Rev 1).nes") == "The Legend of Zelda");
	CHECK(titleFromFileName("Bug's Life, A (USA).gbc") == "A Bug's Life");
	CHECK(titleFromFileName("Lost World, The - Jurassic Park, The (USA).gb") == "The Lost World - Jurassic Park, The");
	CHECK(titleFromFileName("Metal Gear Solid, Theme (USA).gbc") == "Metal Gear Solid, Theme");
}

TEST_CASE("titleFromFileName drops release numbers, site names and underscores") {
	CHECK(titleFromFileName("4273 - Pokemon Mystery Dungeon - Explorers of Sky (US)(XenoPhobia).nds") ==
		"Pokemon Mystery Dungeon - Explorers of Sky");
	CHECK(titleFromFileName("sd:/roms/NDS/br/Final Fantasy IV (BR) - www.romsportugues.com.nds") == "Final Fantasy IV");
	CHECK(titleFromFileName("Chrono Trigger DS (BR) (Sem Crash no DS) (www.romsportugues.com).nds") == "Chrono Trigger DS");
	CHECK(titleFromFileName("The Legend of Zelda \xE2\x80\x93 Spirit Tracks (BR) (www.romsportugues.com) .nds") ==
		"The Legend of Zelda - Spirit Tracks");
	CHECK(titleFromFileName("Game_Name_Here.nds") == "Game Name Here");
	CHECK(titleFromFileName("1942 (Japan, USA).nes") == "1942"); // a year, not a release number
}

TEST_CASE("cleanForFont replaces or drops characters the fonts lack") {
	CHECK(cleanForFont("Sonic & SEGA All-Stars Racing\xE2\x84\xA2") == "Sonic & SEGA All-Stars Racing");
	CHECK(cleanForFont("Disney \xE2\x80\xA2 Pixar") == "Disney \xC2\xB7 Pixar");
	CHECK(cleanForFont("  A \xE2\x80\x94 B\tC  ") == "A - B C");
	CHECK(fitsFont("Pok\xC3\xA9mon"));
	CHECK_FALSE(fitsFont("\xE3\x83\x9E\xE3\x83\xAA\xE3\x82\xAA"));
}

TEST_CASE("parseBannerText joins title lines and keeps the publisher apart") {
	BannerText t = parseBannerText("The Legend of Zelda:\nSpirit Tracks\nNintendo", "The Legend of Zelda - Spirit Tracks");
	CHECK(t.title == "The Legend of Zelda: Spirit Tracks");
	CHECK(t.publisher == "Nintendo");

	t = parseBannerText("Pok\xC3\xA9mon HeartGold\nNintendo", "Pokemon - HeartGold Version");
	CHECK(t.title == "Pok\xC3\xA9mon HeartGold");
	CHECK(t.publisher == "Nintendo");

	t = parseBannerText("Star Wars\xE2\x84\xA2:\nThe Force Unleashed II\xE2\x84\xA2\nLucasArts\xE2\x84\xA2", "");
	CHECK(t.title == "Star Wars: The Force Unleashed II");
	CHECK(t.publisher == "LucasArts");

	t = parseBannerText("Single line", "File");
	CHECK(t.title == "Single line");
	CHECK(t.publisher.empty());
}

TEST_CASE("parseBannerText fixes all-caps and undrawable titles") {
	CHECK(parseBannerText("MARIO KART DS\nNintendo", "Mario Kart DS").title == "Mario Kart DS");
	CHECK(parseBannerText("CRASH\xE2\x84\xA2\nMIND OVER MUTANT\nSierra", "Crash - Mind over Mutant").title ==
		"Crash - Mind over Mutant");
	CHECK(parseBannerText("CHRONO TRIGGER\nSQUARE ENIX", "Chrono Trigger DS").title == "Chrono Trigger");
	CHECK(parseBannerText("FINAL FANTASY IV\nSQUARE ENIX", "Something Else").title == "Final Fantasy IV");
	CHECK(parseBannerText("MARIO & SONIC AT THE\nOLYMPIC WINTER GAMES\nSEGA", "x").title ==
		"Mario & Sonic at the Olympic Winter Games");
	const BannerText jp = parseBannerText("\xE3\x83\x9E\xE3\x83\xAA\xE3\x82\xAA\nHUDSON", "Bomberman Story DS");
	CHECK(jp.title == "Bomberman Story DS");
	CHECK(jp.publisher == "HUDSON");
	CHECK(parseBannerText("", "From File").title == "From File");
}

TEST_CASE("parseBannerText calms partly shouted titles and cleans publishers") {
	CHECK(parseBannerText("RESIDENT EVIL\nDeadly Silence\nCAPCOM", "Resident Evil - Deadly Silence").title ==
		"Resident Evil - Deadly Silence");
	CHECK(parseBannerText("BLEACH\nDark Souls\nSEGA", "Unrelated").title == "Bleach Dark Souls");
	CHECK(parseBannerText("GTA: Chinatown Wars\nROCKSTAR GAMES", "x").title == "GTA: Chinatown Wars");
	CHECK(parseBannerText("Final Fantasy XII\nSQUARE ENIX", "x").title == "Final Fantasy XII");
	CHECK(parseBannerText("Ultimate Mortal Kombat\n\xC2\xA9 2007 Midway", "x").publisher == "Midway");
	CHECK(parseBannerText("Metal Slug 7\n\xC2\xA9SNK PLAYMORE / Ignition", "x").publisher == "SNK PLAYMORE / Ignition");
}

TEST_CASE("titleFromFileName drops scene release suffixes") {
	CHECK(titleFromFileName("5531_-_Dragon_Ball_Kai_Ultimate_Butouden_JPN_NDS-BAHAMUT.nds") == "Dragon Ball Kai Ultimate Butouden");
}

TEST_CASE("parseFileTags reads regions, languages and Portuguese markers") {
	FileTags t = parseFileTags("sd:/roms/NDS/Mario Kart DS (USA) (En,Fr,De,Es,It).nds");
	CHECK(t.region == "USA");
	CHECK(t.languages == "En Fr De Es It");
	CHECK_FALSE(t.portuguese);

	t = parseFileTags("Pokemon - Yellow Version (UE) [C][!].gbc");
	CHECK(t.region == "USA, Europe");
	CHECK_FALSE(t.portuguese);

	CHECK(parseFileTags("Pokemon Blue (UA) [S][BF1].gb").region == "USA, Australia"); // [S] is not Spain

	t = parseFileTags("sd:/roms/SMS/Predator 2 (Brazil) (En).sms");
	CHECK(t.region == "Brazil");
	CHECK_FALSE(t.portuguese); // released in Brazil, in English

	CHECK(parseFileTags("Castlevania Order of Eclesia (BR) (www.romsportugues.com).nds").portuguese);
	CHECK(parseFileTags("Ratatouille (PT) (www.romsportugues.com).nds").portuguese);
	CHECK(parseFileTags("Final Fantasy IV (BR) - www.romsportugues.com.nds").portuguese);
	CHECK(parseFileTags("Game (Brazil) (En,Pt).sms").portuguese);
	CHECK(parseFileTags("sd:/roms/NDS/br/Some Game.nds").portuguese);
	CHECK(parseFileTags("Game [PT-BR].gba").portuguese);
	CHECK_FALSE(parseFileTags("sd:/roms/NDS/brothers/Game (USA).nds").portuguese);
	CHECK_FALSE(parseFileTags("Mario & Luigi - Brothership (USA).nds").portuguese);
}
