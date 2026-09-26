#include <nds.h>
#include <ctime>
#include <set>
#include <string>
#include <vector>

#include "common/systemdetails.h"
#include "core/LibraryScan.h"
#include "launch/TwilightLauncher.h"
#include "my_gurumeditation.h"
#include "platform/Power.h"
#include "platform/RomFiles.h"
#include "platform/Screens.h"
#include "platform/Storage.h"
#include "ui/App.h"
#include "ui/Views.h"

// Read by TWiLight's twlmenusettings.cpp; normally defined in universal/arm9/source/mainAll.cpp.
bool useTwlCfg = false;

namespace {

using namespace dscore;

const std::vector<std::string> kRomRoots = {"sd:/roms/NDS", "sd:/roms/GBA"};
constexpr int kConfigSaveDelayFrames = 120; // batch cursor moves into one SD write
constexpr size_t kProgressEvery = 8;        // redraw the indexing screen every few games

// Milliseconds since boot, from the cascaded timers started in main().
unsigned elapsedMs() {
	return timerTicks2msec(cpuGetTiming());
}

void showMessage(Screens& screens, const std::string& topTitle, const std::vector<std::string>& topLines,
	const std::string& bottomTitle, const std::vector<std::string>& bottomLines) {
	drawMessageScreen(screens.top(), topTitle, topLines);
	drawMessageScreen(screens.bottom(), bottomTitle, bottomLines);
	screens.present();
}

[[noreturn]] void halt(Screens& screens, const std::string& title, const std::vector<std::string>& lines) {
	showMessage(screens, "DSCore", {}, title, lines);
	while (true) swiWaitForVBlank();
}

BannerLanguage systemLanguage() {
	const int language = PersonalData->language;
	return language <= int(BannerLanguage::Spanish) ? BannerLanguage(language) : BannerLanguage::English;
}

// Loads the cached library and brings it up to date with the SD card, showing progress while new
// games are indexed. Appends timings to log.
LibraryData loadLibrary(Screens& screens, std::string& log) {
	LibraryData library;
	unsigned start = elapsedMs();
	const bool cached = storage::loadLibrary(library);
	log += "cache: " + std::string(cached ? "loaded" : "missing") + ", " + std::to_string(library.games.size()) +
	       " games, " + std::to_string(elapsedMs() - start) + " ms\n";

	start = elapsedMs();
	const std::vector<std::string> files = listRomFiles(kRomRoots);
	log += "list: " + std::to_string(files.size()) + " files, " + std::to_string(elapsedMs() - start) + " ms\n";

	std::set<std::string> known;
	for (const GameEntry& game : library.games) known.insert(game.path);
	size_t toIndex = 0;
	for (const std::string& path : files) toIndex += known.count(path) ? 0 : 1;

	start = elapsedMs();
	size_t indexed = 0;
	const BannerLanguage language = systemLanguage();
	const bool changed = applyScan(library, files, [&](const std::string& path, GameEntry& game, std::optional<NdsIcon>& icon) {
		if (indexed % kProgressEvery == 0) {
			const std::string progress = std::to_string(indexed + 1) + " / " + std::to_string(toIndex);
			showMessage(screens, "DSCore", {"Building your library"}, "Indexing games", {progress, path.substr(path.find_last_of('/') + 1)});
		}
		++indexed;
		return readRomInfo(path, language, game, icon);
	});
	log += "index: " + std::to_string(indexed) + " new, " + std::to_string(elapsedMs() - start) + " ms\n";

	if (changed && !storage::saveLibrary(library)) log += "cache: write failed\n";
	return library;
}

// Translates this frame's buttons and touch into actions; returns a ROM path when one should launch.
std::string handleInput(App& app) {
	scanKeys();
	sleepWhileLidClosed();
	const u32 down = keysDown();
	const u32 repeat = keysDownRepeat();
	std::string launch;
	auto apply = [&](Action action, int x = 0, int y = 0) {
		const std::string path = app.handle(action, x, y);
		if (!path.empty()) launch = path;
	};

	if (repeat & KEY_UP) apply(Action::Up);
	if (repeat & KEY_DOWN) apply(Action::Down);
	if (repeat & KEY_LEFT) apply(Action::Left);
	if (repeat & KEY_RIGHT) apply(Action::Right);
	if (down & KEY_A) apply(Action::Launch);
	if (down & KEY_Y) apply(Action::Favorite);
	if (down & KEY_L) apply(Action::PrevTab);
	if (down & KEY_R) apply(Action::NextTab);
	if (down & KEY_SELECT) apply(Action::ToggleView);
	if (down & KEY_START) apply(Action::CycleSort);
	if (down & KEY_TOUCH) {
		touchPosition touch;
		touchRead(&touch);
		apply(Action::Tap, touch.px, touch.py);
	}
	return launch;
}

// Drawing cost of the frames since boot, logged before each launch.
struct DrawStats {
	unsigned frames = 0;
	unsigned totalMs = 0;
	unsigned maxMs = 0;

	void add(unsigned ms) {
		++frames;
		totalMs += ms;
		if (ms > maxMs) maxMs = ms;
	}
	std::string summary() const {
		if (frames == 0) return "redraws: none\n";
		return "redraws: " + std::to_string(frames) + ", avg " + std::to_string(totalMs / frames) + " ms, max " +
		       std::to_string(maxMs) + " ms\n";
	}
};

void launch(Screens& screens, App& app, UserData& userData, Config& config, const std::string& path) {
	userData.recordLaunch(path, uint32_t(time(nullptr)));
	storage::saveUserData(userData);
	config.selectedPath = path;
	storage::saveConfig(config);

	const GameEntry* game = app.selected();
	showMessage(screens, "Starting", {game ? game->title : path}, "", {"Loading through TWiLight Menu++..."});
	int code = 0;
	const LaunchError error = launchViaTwilight(path, &code);
	showMessage(screens, "Could not start the game", {describe(error), "code " + std::to_string(code)}, "", {"Press B to go back"});
	do {
		swiWaitForVBlank();
		scanKeys();
	} while (!(keysDown() & KEY_B));
}

} // namespace

int main(int argc, char** argv) {
	myExceptionHandler();
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);
	cpuStartTiming(0);
	sys().initFilesystem(argc > 0 ? argv[0] : "sd:/dscore.nds");
	sys().initArm7RegStatuses();

	// main.srldr fades both screens to white before booting its theme and leaves the fade-in to it.
	setBrightness(3, 0);
	Screens screens;
	if (!sys().fatInitOk()) halt(screens, "SD card not found", {"DSCore needs the DSi SD card."});
	showMessage(screens, "DSCore", {"Loading..."}, "", {});
	storage::ensureDataDir();

	std::string log = "DSCore boot\n";
	Config config = storage::loadConfig();
	UserData userData = storage::loadUserData();
	const LibraryData library = loadLibrary(screens, log);
	App app(library, userData, config);
	log += "ready: " + std::to_string(elapsedMs()) + " ms since start\n";

	// One full redraw of both screens: the cost of every cursor move.
	const unsigned drawStart = elapsedMs();
	app.drawTop(screens.top());
	app.drawBottom(screens.bottom());
	log += "draw: " + std::to_string(elapsedMs() - drawStart) + " ms\n";
	screens.present();
	app.takeRedraw();
	storage::writeBootLog(log);

	keysSetRepeat(15, 4);
	int configSaveCountdown = -1;
	DrawStats drawStats;
	while (true) {
		if (powerButtonPressed()) {
			if (configSaveCountdown > 0) storage::saveConfig(config);
			returnToSystemMenu();
		}
		const std::string path = handleInput(app);
		if (!path.empty()) {
			storage::appendLog(drawStats.summary());
			launch(screens, app, userData, config, path);
			configSaveCountdown = -1;
			app.invalidate();
			continue;
		}
		if (app.takeUserDataChanged()) storage::saveUserData(userData);
		if (app.takeConfigChanged()) configSaveCountdown = kConfigSaveDelayFrames;
		if (configSaveCountdown > 0 && --configSaveCountdown == 0) storage::saveConfig(config);

		if (app.takeRedraw()) {
			const unsigned start = elapsedMs();
			app.drawTop(screens.top());
			app.drawBottom(screens.bottom());
			drawStats.add(elapsedMs() - start);
			screens.present();
		} else {
			swiWaitForVBlank();
		}
	}
}
