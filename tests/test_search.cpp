#include "doctest.h"

#include <string>
#include <vector>

#include "core/Library.h"
#include "core/Search.h"
#include "ui/Keyboard.h"

using namespace dscore;

TEST_CASE("matchesQuery ignores case and Latin-1 accents") {
	CHECK(matchesQuery("Pok\xC3\xA9mon HeartGold", "pokemon"));
	CHECK(matchesQuery("Pok\xC3\xA9mon HeartGold", "HEART"));
	CHECK(matchesQuery("Mario Kart DS", "kart ds"));
	CHECK(matchesQuery("anything", ""));
	CHECK_FALSE(matchesQuery("Mario Kart DS", "zelda"));
}

TEST_CASE("libraryView keeps only games matching the query") {
	const std::vector<GameEntry> games = {
		{"sd:/roms/NDS/a.nds", "Mario Kart DS", System::Nds, "", 0, -1},
		{"sd:/roms/GBA/b.gba", "Mario & Luigi", System::Gba, "", 0, -1},
		{"sd:/roms/NDS/c.nds", "Zelda", System::Nds, "", 0, -1},
	};
	const UserData data;
	CHECK(libraryView(games, data, Tab::All, SortKey::Name, "mario").size() == 2);
	CHECK(libraryView(games, data, Tab::Nds, SortKey::Name, "mario").size() == 1);
	CHECK(libraryView(games, data, Tab::All, SortKey::Name, "").size() == 3);
}

TEST_CASE("keyboard types characters, spaces and deletes") {
	std::string query;
	CHECK(applyKey(keyboardKeys()[keyIndexFor('M')], query) == KeyResult::Edited);
	CHECK(applyKey(keyboardKeys()[keyIndexFor('A')], query) == KeyResult::Edited);
	CHECK(applyKey(keyboardKeys()[keyIndexFor(' ')], query) == KeyResult::Edited);
	CHECK(query == "MA ");
	CHECK(applyKey(keyboardKeys()[keyIndexFor('\b')], query) == KeyResult::Edited);
	CHECK(query == "MA");
	CHECK(applyKey(keyboardKeys()[keyIndexFor('\n')], query) == KeyResult::Done);
}

TEST_CASE("keyboard navigation stays on the keyboard and keeps the column") {
	const int q = keyIndexFor('Q');
	const int one = keyIndexFor('1');
	CHECK(moveKey(q, Move::Up) == one);
	CHECK(moveKey(one, Move::Up) == one);          // top row: no move
	CHECK(moveKey(q, Move::Left) == q);             // left edge: no move
	CHECK(moveKey(keyIndexFor('P'), Move::Right) == keyIndexFor('P'));
	CHECK(moveKey(keyIndexFor('W'), Move::Down) == keyIndexFor('S'));
	const int space = keyIndexFor(' ');
	CHECK(moveKey(keyIndexFor('C'), Move::Down) == space); // bottom row: nearest wide key
}

TEST_CASE("keyboard hit test finds the touched key") {
	const Key& key = keyboardKeys()[keyIndexFor('G')];
	CHECK(keyAt(key.rect.x + 2, key.rect.y + 2) == keyIndexFor('G'));
	CHECK(keyAt(0, 0) == -1);
}
