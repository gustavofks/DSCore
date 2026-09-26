#include "doctest.h"

#include <string>
#include <vector>

#include "ui/App.h"
#include "ui/Layout.h"

using namespace dscore;

namespace {

LibraryData library(size_t ds, size_t gba) {
	LibraryData lib;
	for (size_t i = 0; i < ds; ++i) {
		const std::string n = std::to_string(100 + i);
		lib.games.push_back({"sd:/roms/NDS/" + n + ".nds", "DS " + n, System::Nds, "", 0, -1});
	}
	for (size_t i = 0; i < gba; ++i) {
		const std::string n = std::to_string(100 + i);
		lib.games.push_back({"sd:/roms/GBA/" + n + ".gba", "GBA " + n, System::Gba, "", 0, -1});
	}
	return lib;
}

} // namespace

TEST_CASE("App restores the saved selection and remembers moves") {
	const LibraryData lib = library(3, 2);
	UserData data;
	Config config;
	config.selectedPath = "sd:/roms/NDS/102.nds";
	App app(lib, data, config);
	REQUIRE(app.selected() != nullptr);
	CHECK(app.selected()->path == "sd:/roms/NDS/102.nds");

	app.handle(Action::Right);
	CHECK(app.selected()->path == "sd:/roms/GBA/100.gba");
	CHECK(config.selectedPath == "sd:/roms/GBA/100.gba");
	CHECK(app.takeConfigChanged());
	CHECK_FALSE(app.takeConfigChanged());
}

TEST_CASE("App launches the selected game with A") {
	const LibraryData lib = library(2, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	CHECK(app.handle(Action::Launch) == "sd:/roms/NDS/100.nds");
}

TEST_CASE("App switches tabs and keeps the selection when it is listed") {
	const LibraryData lib = library(2, 2);
	UserData data;
	Config config;
	config.selectedPath = "sd:/roms/GBA/101.gba";
	App app(lib, data, config);
	app.handle(Action::NextTab); // Favorites: empty
	CHECK(config.tab == Tab::Favorites);
	CHECK(app.selected() == nullptr);
	app.handle(Action::NextTab); // DS
	app.handle(Action::NextTab); // GBA
	CHECK(config.tab == Tab::Gba);
	app.handle(Action::PrevTab);
	app.handle(Action::NextTab);
	CHECK(app.selected()->system == System::Gba);
}

TEST_CASE("App starts a tab at its first game when the selection is not in it") {
	const LibraryData lib = library(3, 2);
	UserData data;
	Config config;
	config.selectedPath = "sd:/roms/NDS/102.nds";
	App app(lib, data, config);
	app.handle(Action::NextTab); // Favorites (empty)
	app.handle(Action::NextTab); // DS: selection is listed
	CHECK(app.selected()->path == "sd:/roms/NDS/102.nds");
	app.handle(Action::NextTab); // GBA: selection is not listed
	CHECK(app.selected()->path == "sd:/roms/GBA/100.gba");
}

TEST_CASE("App toggles favorites and reports user data changes") {
	const LibraryData lib = library(2, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::Favorite);
	CHECK(data.find("sd:/roms/NDS/100.nds")->favorite);
	CHECK(app.takeUserDataChanged());

	config.tab = Tab::Favorites;
	App favorites(lib, data, config);
	favorites.handle(Action::Favorite);
	CHECK(favorites.selected() == nullptr); // un-favorited game leaves the tab
}

TEST_CASE("App cycles sort and view") {
	const LibraryData lib = library(2, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::CycleSort);
	CHECK(config.sort == SortKey::System);
	app.handle(Action::ToggleView);
	CHECK(config.view == ViewMode::List);
}

TEST_CASE("App touch selects a cell, then launches it, and switches tabs") {
	const LibraryData lib = library(3, 1);
	UserData data;
	Config config;
	App app(lib, data, config);
	const Rect cell = layout::gridCellRect(1);
	CHECK(app.handle(Action::Tap, cell.x + 5, cell.y + 5).empty());
	CHECK(app.selected()->path == "sd:/roms/NDS/101.nds");
	CHECK(app.handle(Action::Tap, cell.x + 5, cell.y + 5) == "sd:/roms/NDS/101.nds");

	const Rect gbaTab = layout::tabRect(int(Tab::Gba));
	app.handle(Action::Tap, gbaTab.x + 2, gbaTab.y + 2);
	CHECK(config.tab == Tab::Gba);
	CHECK(app.selected()->system == System::Gba);
}

TEST_CASE("App draws both screens without crashing on an empty library") {
	const LibraryData lib;
	UserData data;
	Config config;
	App app(lib, data, config);
	std::vector<uint16_t> pixels(layout::kScreenW * layout::kScreenH);
	Canvas canvas(pixels.data(), layout::kScreenW, layout::kScreenH);
	app.drawTop(canvas);
	app.drawBottom(canvas);
	CHECK(app.handle(Action::Launch).empty());
}
