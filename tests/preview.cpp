// Host preview: builds the library from real ROM folders with the same code the DS runs, then writes
// both screens for a few UI states as PNG files (top screen above the bottom one).
//
// Usage: build/preview <out-dir> <userdata.ini> <covers-dir> <rom-dir>...
// Run it from a folder where "sd:" leads to the SD card copy, so paths match the ones on the DS.

#include <cstdio>
#include <string>
#include <vector>

#include "common/lodepng.h"
#include "core/Cover.h"
#include "core/LaunchKeys.h"
#include "core/LibraryScan.h"
#include "platform/FileIo.h"
#include "platform/RomFiles.h"
#include "ui/App.h"
#include "ui/Keyboard.h"
#include "ui/Layout.h"
#include "ui/Views.h"

using namespace dscore;

namespace {

constexpr int kW = layout::kScreenW;
constexpr int kH = layout::kScreenH;
constexpr int kGap = 8;

void writePng(const std::string& path, const std::vector<uint16_t>& top, const std::vector<uint16_t>& bottom) {
	const int height = kH * 2 + kGap;
	std::vector<uint8_t> rgb(size_t(kW * height * 3), 0x40); // gap color
	auto put = [&](int x, int y, uint16_t c) {
		uint8_t* p = &rgb[size_t((y * kW + x) * 3)];
		p[0] = uint8_t((c & 31) * 255 / 31);
		p[1] = uint8_t(((c >> 5) & 31) * 255 / 31);
		p[2] = uint8_t(((c >> 10) & 31) * 255 / 31);
	};
	for (int y = 0; y < kH; ++y) {
		for (int x = 0; x < kW; ++x) {
			put(x, y, top[size_t(y * kW + x)]);
			put(x, y + kH + kGap, bottom[size_t(y * kW + x)]);
		}
	}
	lodepng::encode(path, rgb, unsigned(kW), unsigned(height), LCT_RGB);
	std::printf("wrote %s\n", path.c_str());
}

} // namespace

int main(int argc, char** argv) {
	if (argc < 5) {
		std::fprintf(stderr, "usage: %s <out-dir> <userdata.ini> <covers-dir> <rom-dir>...\n", argv[0]);
		return 1;
	}
	const std::string outDir = argv[1];
	const std::string userDataPath = argv[2];
	const std::string coversDir = argv[3];
	const std::vector<std::string> roots(argv + 4, argv + argc);

	LibraryData library;
	applyScan(library, listRomFiles(roots), [](const std::string& path, GameEntry& game, std::optional<NdsIcon>& icon) {
		return readRomInfo(path, BannerLanguage::English, game, icon);
	});
	std::printf("%zu games, %zu icons\n", library.games.size(), library.icons.size());

	std::string ini;
	readFile(userDataPath, ini);
	UserData userData = UserData::parse(ini);

	std::vector<uint16_t> top(kW * kH), bottom(kW * kH);
	Canvas topCanvas(top.data(), kW, kH), bottomCanvas(bottom.data(), kW, kH);
	// Loads what main.cpp's CoverLoader would for the selected game, then draws both screens.
	auto shot = [&](App& app, const std::string& name) {
		if (const GameEntry* game = app.selected()) {
			std::string data;
			std::optional<Cover> cover;
			if (readFile(coversDir + "/" + coverFileName(game->path), data)) {
				cover = decodeCover(reinterpret_cast<const uint8_t*>(data.data()), data.size());
			}
			app.setCover(game->path, std::move(cover));
			bool hasSave = false;
			for (const std::string& save : saveFileCandidates(game->path)) hasSave = hasSave || fileExists(save);
			app.setHasSave(game->path, hasSave);
		}
		app.drawTop(topCanvas);
		app.drawBottom(bottomCanvas);
		writePng(outDir + "/" + name + ".png", top, bottom);
	};
	auto pathOf = [&](const std::string& needle) {
		for (const GameEntry& game : library.games) {
			if (game.path.find(needle) != std::string::npos) return game.path;
		}
		return std::string();
	};

	Config config;
	config.selectedPath = pathOf("HeartGold");
	App app(library, userData, config);
	app.setThemes(builtInThemes());
	shot(app, "01-all-grid");

	config.selectedPath = pathOf("br/The Legend of Zelda");
	App br(library, userData, config);
	shot(br, "02-br-detail");

	config.selectedPath = pathOf("Star Wars - The Force Unleashed II");
	App longTitle(library, userData, config);
	shot(longTitle, "03-long-title");

	config.selectedPath = pathOf("Metroid II");
	config.tab = Tab::console(System::Gb);
	App gb(library, userData, config);
	shot(gb, "04-gb-tab-save");

	config.selectedPath.clear();
	config.tab = Tab::all();
	config.filter = Filter::Portuguese;
	config.view = ViewMode::List;
	App portuguese(library, userData, config);
	shot(portuguese, "05-portuguese-list");

	config.filter = Filter::Played;
	config.sort = SortKey::Recent;
	config.view = ViewMode::Grid;
	App recent(library, userData, config);
	shot(recent, "06-played-recent");

	config = Config{};
	config.tab = Tab::console(System::Atari2600);
	App last(library, userData, config);
	shot(last, "07-tabs-scrolled");

	app.handle(Action::Menu);
	shot(app, "08-menu");
	for (int i = 0; i < 5; ++i) app.handle(Action::Down);
	app.handle(Action::Launch);
	shot(app, "09-menu-consoles");
	app.handle(Action::Back);
	app.handle(Action::Back);

	app.handle(Action::Search);
	for (char c : std::string("MARIO")) {
		const Rect& key = keyboardKeys()[size_t(keyIndexFor(c))].rect;
		app.handle(Action::Tap, key.x + 2, key.y + 2);
	}
	shot(app, "10-search");

	drawMessageScreen(topCanvas, builtInThemes()[0], "DSCore", {"Loading library..."});
	drawMessageScreen(bottomCanvas, builtInThemes()[0], "Indexing games", {"42 / 391", "Super Mario 64 DS"});
	writePng(outDir + "/11-indexing.png", top, bottom);
	return 0;
}
