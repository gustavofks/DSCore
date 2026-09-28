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

TEST_CASE("App reports interface sounds") {
	const LibraryData lib = library(20, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::Right);
	CHECK(app.takeSound() == Sound::Move);
	CHECK(app.takeSound() == Sound::None);
	app.handle(Action::NextTab);
	CHECK(app.takeSound() == Sound::Select);
	app.handle(Action::PrevTab);
	app.takeSound();
	CHECK_FALSE(app.handle(Action::Launch).empty());
	CHECK(app.takeSound() == Sound::Launch);
	app.handle(Action::Menu);
	CHECK(app.takeSound() == Sound::Select);
	app.handle(Action::Back);
	CHECK(app.takeSound() == Sound::Back);
}

TEST_CASE("App asks for a bottom transition when the page changes, not when the cursor moves") {
	const LibraryData lib = library(40, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	std::vector<uint16_t> pixels(layout::kScreenW * layout::kScreenH);
	Canvas canvas(pixels.data(), layout::kScreenW, layout::kScreenH);
	app.drawBottom(canvas);
	CHECK_FALSE(app.takeBottomTransition()); // first draw: nothing to transition from
	app.handle(Action::Right);
	app.drawBottom(canvas);
	CHECK_FALSE(app.takeBottomTransition()); // same page
	for (int i = 0; i < 3; ++i) app.handle(Action::Down);
	app.drawBottom(canvas);
	CHECK(app.takeBottomTransition()); // next page
}

TEST_CASE("App options menu toggles sounds") {
	const LibraryData lib = library(1, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::Menu);
	for (int i = 0; i < 3; ++i) app.handle(Action::Down);
	app.handle(Action::Launch);
	CHECK_FALSE(config.sound);
}

TEST_CASE("App toggles the view with SELECT") {
	const LibraryData lib = library(2, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::ToggleView);
	CHECK(config.view == ViewMode::List);
}

TEST_CASE("App options menu changes sort, view and theme and requests a rebuild") {
	const LibraryData lib = library(2, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.setThemes(builtInThemes());
	app.handle(Action::Menu);
	CHECK(app.menuOpen());
	CHECK(app.handle(Action::Launch).empty()); // row 0: sort
	CHECK(config.sort == SortKey::System);
	app.handle(Action::Left);
	CHECK(config.sort == SortKey::Name);

	app.handle(Action::Down); // view
	app.handle(Action::Right);
	CHECK(config.view == ViewMode::List);

	app.handle(Action::Down); // theme
	app.handle(Action::Right);
	CHECK(config.theme == builtInThemes()[1].name);
	CHECK(&app.theme() == &builtInThemes()[1]);
	app.handle(Action::Left);
	app.handle(Action::Left); // wraps around to the last theme
	CHECK(config.theme == builtInThemes().back().name);

	app.handle(Action::Down); // sounds
	app.handle(Action::Down); // rebuild
	app.handle(Action::Launch);
	CHECK_FALSE(app.menuOpen());
	CHECK(app.takeRebuildRequest());
	CHECK_FALSE(app.takeRebuildRequest());
}

TEST_CASE("App restores the configured theme and closes the menu with B") {
	const LibraryData lib = library(1, 0);
	UserData data;
	Config config;
	config.theme = "OLED";
	App app(lib, data, config);
	app.setThemes(builtInThemes());
	CHECK(app.theme().name == "OLED");
	app.handle(Action::Menu);
	app.handle(Action::Back);
	CHECK_FALSE(app.menuOpen());
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

TEST_CASE("App partial redraws match a full redraw") {
	const LibraryData lib = library(20, 5);
	UserData data;
	for (ViewMode mode : {ViewMode::Grid, ViewMode::List}) {
		Config config;
		config.view = mode;
		App app(lib, data, config);
		std::vector<uint16_t> kept(layout::kScreenW * layout::kScreenH), fresh(kept.size());
		Canvas keptCanvas(kept.data(), layout::kScreenW, layout::kScreenH);
		Canvas freshCanvas(fresh.data(), layout::kScreenW, layout::kScreenH);
		app.drawBottom(keptCanvas);
		for (Action move : {Action::Right, Action::Down, Action::Down, Action::Left, Action::Down, Action::Up}) {
			app.handle(move);
			app.drawBottom(keptCanvas); // partial when the page did not change

			Config sameConfig = config;
			App reference(lib, data, sameConfig);
			reference.drawBottom(freshCanvas);
			CHECK(kept == fresh);
		}
	}
}

TEST_CASE("App redraws everything after invalidate") {
	const LibraryData lib = library(3, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	std::vector<uint16_t> pixels(layout::kScreenW * layout::kScreenH);
	Canvas canvas(pixels.data(), layout::kScreenW, layout::kScreenH);
	app.drawBottom(canvas);
	app.takeRedraw();
	canvas.fill(0x801F); // something else drew over the screen
	app.invalidate();
	CHECK(app.takeRedraw());
	app.drawBottom(canvas);
	CHECK(pixels[size_t(layout::kScreenW) * layout::kScreenH / 2] != 0x801F);
}

TEST_CASE("App shows a cover only while its game is selected") {
	const LibraryData lib = library(2, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	std::vector<uint16_t> withCover(layout::kScreenW * layout::kScreenH), without(withCover.size());
	Canvas withCanvas(withCover.data(), layout::kScreenW, layout::kScreenH);
	Canvas withoutCanvas(without.data(), layout::kScreenW, layout::kScreenH);

	app.drawTop(withoutCanvas);
	app.takeRedraw();
	Cover cover;
	cover.width = cover.height = 4;
	cover.pixels.assign(16, 0x801F);
	app.setCover("sd:/roms/NDS/100.nds", cover);
	CHECK(app.takeRedraw());
	app.drawTop(withCanvas);
	CHECK(withCover != without);

	app.handle(Action::Right); // another game is selected now
	app.takeRedraw();
	app.setCover("sd:/roms/NDS/100.nds", std::nullopt); // not the selected game: no redraw needed
	CHECK_FALSE(app.takeRedraw());
}

#include "ui/Keyboard.h"

namespace {

void tapKey(App& app, char value) {
	const Rect& r = keyboardKeys()[size_t(keyIndexFor(value))].rect;
	app.handle(Action::Tap, r.x + 2, r.y + 2);
}

} // namespace

TEST_CASE("App search filters as you type and keeps the filter after OK") {
	LibraryData lib;
	lib.games.push_back({"sd:/roms/NDS/a.nds", "Mario Kart DS", System::Nds, "", 0, -1});
	lib.games.push_back({"sd:/roms/NDS/b.nds", "Zelda", System::Nds, "", 0, -1});
	lib.games.push_back({"sd:/roms/GBA/c.gba", "Metroid Fusion", System::Gba, "", 0, -1});
	UserData data;
	Config config;
	App app(lib, data, config);

	app.handle(Action::Search);
	CHECK(app.searching());
	tapKey(app, 'Z');
	CHECK(app.query() == "Z");
	CHECK(app.selected()->title == "Zelda");
	CHECK(app.handle(Action::Launch).empty()); // A types the key under the cursor, it never launches
	CHECK(app.query() == "ZZ");
	app.handle(Action::Back);                  // B deletes
	CHECK(app.query() == "Z");

	tapKey(app, '\n'); // OK
	CHECK_FALSE(app.searching());
	CHECK(app.query() == "Z");
	CHECK(app.handle(Action::Launch) == "sd:/roms/NDS/b.nds");

	app.handle(Action::Back); // clears the filter
	CHECK(app.query().empty());
	CHECK(app.selected() != nullptr);
}

TEST_CASE("App search: B deletes, then leaves; the D-pad moves between keys") {
	const LibraryData lib = library(3, 0);
	UserData data;
	Config config;
	App app(lib, data, config);
	app.handle(Action::Search);
	app.handle(Action::Launch); // types the key under the cursor ('A' at start)
	CHECK(app.query() == "A");
	app.handle(Action::Right);
	app.handle(Action::Launch);
	CHECK(app.query() == "AS");
	app.handle(Action::Back);
	CHECK(app.query() == "A");
	app.handle(Action::Back);
	app.handle(Action::Back); // nothing left to delete: leaves the keyboard
	CHECK_FALSE(app.searching());
	CHECK(app.query().empty());
}
