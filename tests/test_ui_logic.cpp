#include "doctest.h"

#include "core/Config.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"

using namespace dscore;

TEST_CASE("Config round-trips and ignores invalid values") {
	Config config;
	config.tab = Tab::Gba;
	config.sort = SortKey::MostPlayed;
	config.view = ViewMode::List;
	config.selectedPath = "sd:/roms/GBA/Metroid = Fusion.gba";
	config.theme = "OLED";
	config.sound = false;
	const Config copy = Config::parse(config.serialize());
	CHECK(copy.theme == "OLED");
	CHECK_FALSE(copy.sound);
	CHECK(Config{}.sound);
	CHECK(copy.tab == Tab::Gba);
	CHECK(copy.sort == SortKey::MostPlayed);
	CHECK(copy.view == ViewMode::List);
	CHECK(copy.selectedPath == "sd:/roms/GBA/Metroid = Fusion.gba");

	const Config bad = Config::parse("[DSCORE]\nTAB = 9\nSORT = x\nVIEW = 1\n");
	CHECK(bad.tab == Tab::All);
	CHECK(bad.sort == SortKey::Name);
	CHECK(bad.view == ViewMode::List);
}

TEST_CASE("layout covers the bottom screen with tabs and a 5x3 grid") {
	CHECK(layout::tabAt(0, 0) == 0);
	CHECK(layout::tabAt(255, 19) == 4);
	CHECK(layout::tabAt(10, 30) == -1);
	const Rect first = layout::gridCellRect(0);
	const Rect last = layout::gridCellRect(layout::kGridPerPage - 1);
	CHECK(layout::gridSlotAt(first.x + 1, first.y + 1) == 0);
	CHECK(layout::gridSlotAt(last.x + last.w - 1, last.y + last.h - 1) == layout::kGridPerPage - 1);
	CHECK(last.y + last.h <= layout::kScreenH - layout::kFooterH);
	CHECK(layout::gridSlotAt(128, 191) == -1);
	CHECK(layout::listRowAt(5, layout::kContentY) == 0);
}

TEST_CASE("grid navigation moves by item and by row, clamped") {
	CHECK(moveCursor(0, 20, ViewMode::Grid, Move::Right) == 1);
	CHECK(moveCursor(0, 20, ViewMode::Grid, Move::Left) == 0);
	CHECK(moveCursor(2, 20, ViewMode::Grid, Move::Down) == 7);
	CHECK(moveCursor(17, 20, ViewMode::Grid, Move::Down) == 19);
	CHECK(moveCursor(3, 20, ViewMode::Grid, Move::Up) == 0);
	CHECK(moveCursor(5, 0, ViewMode::Grid, Move::Down) == 0);
}

TEST_CASE("list navigation moves by row and by page") {
	const size_t page = pageSize(ViewMode::List);
	CHECK(moveCursor(0, 100, ViewMode::List, Move::Down) == 1);
	CHECK(moveCursor(0, 100, ViewMode::List, Move::Right) == page);
	CHECK(moveCursor(page + 2, 100, ViewMode::List, Move::Left) == 2);
	CHECK(moveCursor(98, 100, ViewMode::List, Move::Right) == 99);
}

TEST_CASE("pages start at multiples of the page size") {
	CHECK(pageSize(ViewMode::Grid) == 15);
	CHECK(pageStart(14, ViewMode::Grid) == 0);
	CHECK(pageStart(15, ViewMode::Grid) == 15);
	CHECK(pageStart(31, ViewMode::Grid) == 30);
}
