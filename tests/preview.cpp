// Host preview: builds the library from real ROM folders with the same code the DS runs, then writes
// both screens for a few UI states as PNG files (top screen above the bottom one).
//
// Usage: build/preview <out-dir> <twilight-extras-dir> <rom-dir>...

#include <cstdio>
#include <string>
#include <vector>

#include "common/lodepng.h"
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
	if (argc < 4) {
		std::fprintf(stderr, "usage: %s <out-dir> <twilight-extras-dir> <rom-dir>...\n", argv[0]);
		return 1;
	}
	const std::string outDir = argv[1];
	const std::string extras = argv[2];
	const std::vector<std::string> roots(argv + 3, argv + argc);

	LibraryData library;
	applyScan(library, listRomFiles(roots), [](const std::string& path, GameEntry& game, std::optional<NdsIcon>& icon) {
		return readRomInfo(path, BannerLanguage::English, game, icon);
	});
	std::printf("%zu games, %zu icons\n", library.games.size(), library.icons.size());

	UserData userData;
	std::string recent, times;
	readFile(extras + "/recentlyplayed.ini", recent);
	readFile(extras + "/timesplayed.ini", times);
	userData.importTwilightHistory(recent, times);
	if (!library.games.empty()) userData.toggleFavorite(library.games[0].path);
	if (!library.games.empty()) userData.recordLaunch(library.games[0].path, 1790380800);

	std::vector<uint16_t> top(kW * kH), bottom(kW * kH);
	Canvas topCanvas(top.data(), kW, kH), bottomCanvas(bottom.data(), kW, kH);
	auto shot = [&](App& app, const std::string& name) {
		app.drawTop(topCanvas);
		app.drawBottom(bottomCanvas);
		writePng(outDir + "/" + name + ".png", top, bottom);
	};

	Config config;
	App app(library, userData, config);
	shot(app, "1-grid");
	for (int i = 0; i < 6; ++i) app.handle(Action::Right);
	shot(app, "2-grid-moved");
	app.handle(Action::NextTab);
	shot(app, "3-favorites");
	app.handle(Action::NextTab);
	app.handle(Action::NextTab);
	shot(app, "4-gba");
	app.handle(Action::ToggleView);
	app.handle(Action::NextTab);
	shot(app, "5-recent-list");

	app.handle(Action::ToggleView);
	app.handle(Action::NextTab); // back to All
	app.handle(Action::Search);
	for (char c : std::string("MARIO")) {
		const Rect& key = keyboardKeys()[size_t(keyIndexFor(c))].rect;
		app.handle(Action::Tap, key.x + 2, key.y + 2);
	}
	shot(app, "7-search");
	app.handle(Action::CycleSort); // START finishes the search
	shot(app, "8-search-results");

	drawMessageScreen(topCanvas, "DSCore", {"Loading library..."});
	drawMessageScreen(bottomCanvas, "Indexing games", {"42 / 324", "Super Mario 64 DS"});
	writePng(outDir + "/6-indexing.png", top, bottom);
	return 0;
}
