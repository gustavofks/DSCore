#include "doctest.h"

#include "core/Config.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"

using namespace dscore;

TEST_CASE("Config round-trips and ignores invalid values") {
	Config config;
	config.tab = Tab::console(System::Gba);
	config.sort = SortKey::MostPlayed;
	config.view = ViewMode::List;
	config.selectedPath = "sd:/roms/GBA/Metroid = Fusion.gba";
	config.theme = "OLED";
	config.sound = false;
	const Config copy = Config::parse(config.serialize());
	CHECK(copy.theme == "OLED");
	CHECK_FALSE(copy.sound);
	CHECK(Config{}.sound);
	CHECK(copy.tab == Tab::console(System::Gba));
	CHECK(copy.sort == SortKey::MostPlayed);
	CHECK(copy.view == ViewMode::List);
	CHECK(copy.selectedPath == "sd:/roms/GBA/Metroid = Fusion.gba");

	CHECK(config.serialize().find("TAB = gba\n") != std::string::npos);
	CHECK(Config::parse("[DSCORE]\nTAB = 3\n").tab == Tab::console(System::Gba)); // index from older versions
	CHECK(Config::parse("[DSCORE]\nTAB = snes\n").tab == Tab::console(System::Snes));
	CHECK(copy.filter == Filter::All);

	// Favorites and Recent were tabs before 0.4; they become a filter (and a sort order).
	Config old = Config::parse("[DSCORE]\nTAB = favorites\nSORT = 2\n");
	CHECK(old.tab == Tab::all());
	CHECK(old.filter == Filter::Favorites);
	CHECK(old.sort == SortKey::MostPlayed);
	old = Config::parse("[DSCORE]\nTAB = 4\nSORT = 0\n");
	CHECK(old.tab == Tab::all());
	CHECK(old.filter == Filter::Played);
	CHECK(old.sort == SortKey::Recent);
	CHECK(Config::parse("[DSCORE]\nTAB = 1\n").filter == Filter::Favorites);
	CHECK(Config::parse("[DSCORE]\nTAB = recent\nFILTER = portuguese\n").filter == Filter::Portuguese);

	Config hidden;
	hidden.hiddenSystems = (1u << int(System::Gb)) | (1u << int(System::Atari2600));
	hidden.filter = Filter::NotPlayed;
	const Config hiddenCopy = Config::parse(hidden.serialize());
	CHECK(hidden.serialize().find("HIDDEN = gb,a26\n") != std::string::npos);
	CHECK(hiddenCopy.hiddenSystems == hidden.hiddenSystems);
	CHECK(hiddenCopy.filter == Filter::NotPlayed);

	const Config bad = Config::parse("[DSCORE]\nTAB = 9\nSORT = x\nVIEW = 1\n");
	CHECK(bad.tab == Tab::all());
	CHECK(bad.sort == SortKey::Name);
	CHECK(bad.view == ViewMode::List);
}

TEST_CASE("layout covers the bottom screen with tabs and a 5x3 grid") {
	const std::vector<Rect> tabs = layout::tabRects({18, 18, 12, 18, 36}, 0);
	CHECK(layout::tabAt(tabs, 0, 0) == 0);
	CHECK(layout::tabAt(tabs, 255, 19) == 4);
	CHECK(layout::tabAt(tabs, 10, 30) == -1);
	CHECK(tabs[0].w == tabs[1].w); // labels fit: even widths
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

TEST_CASE("tab bar shares the width, then scrolls to keep the active tab visible") {
	// Too wide for even widths, narrow enough to fit: natural widths plus an equal share of the rest.
	const std::vector<Rect> fit = layout::tabRects({18, 18, 12, 18, 18, 24, 36}, 0);
	CHECK(fit.front().x == 0);
	CHECK(fit.back().x + fit.back().w == layout::kScreenW);
	CHECK(fit[5].w > fit[2].w);

	// Too wide for the screen: the active tab is scrolled into view, the first and last stay reachable.
	const std::vector<int> many = {18, 18, 12, 18, 12, 18, 18, 24, 18, 12, 12, 36};
	const std::vector<Rect> start = layout::tabRects(many, 0);
	CHECK(start.front().x == 0);
	CHECK(start.back().x + start.back().w > layout::kScreenW);
	const std::vector<Rect> end = layout::tabRects(many, int(many.size()) - 1);
	CHECK(end.back().x + end.back().w == layout::kScreenW);
	for (int active = 0; active < int(many.size()); ++active) {
		const Rect r = layout::tabRects(many, active)[size_t(active)];
		CHECK(r.x >= 0);
		CHECK(r.x + r.w <= layout::kScreenW);
	}
	CHECK(layout::tabAt(start, -1, 5) == -1);
}
