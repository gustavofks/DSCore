#include <nds.h>
#include <ctime>
#include <set>
#include <string>
#include <vector>

#include "common/systemdetails.h"
#include "core/LibraryScan.h"
#include "core/Version.h"
#include "launch/TwilightLauncher.h"
#include "my_gurumeditation.h"
#include "platform/Effects.h"
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
constexpr int kCoverDelayFrames = 8;        // load box art once the cursor rests, not while scrolling
constexpr int kFadeFrames = 8;

// Theme of the message screens (loading, errors); follows the one chosen in the options menu.
const Theme* messageTheme = &builtInThemes()[0];

// Milliseconds since boot, from the cascaded timers started in main().
unsigned elapsedMs() {
	return timerTicks2msec(cpuGetTiming());
}

void showMessage(Screens& screens, const std::string& topTitle, const std::vector<std::string>& topLines,
	const std::string& bottomTitle, const std::vector<std::string>& bottomLines) {
	drawMessageScreen(screens.top(), *messageTheme, topTitle, topLines);
	drawMessageScreen(screens.bottom(), *messageTheme, bottomTitle, bottomLines);
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

// Loads the cached library (unless rebuilding) and brings it up to date with the SD card, showing
// progress while new games are indexed. Appends timings to log.
LibraryData loadLibrary(Screens& screens, std::string& log, bool rebuild = false) {
	LibraryData library;
	unsigned start = elapsedMs();
	const bool cached = !rebuild && storage::loadLibrary(library);
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
	if (down & KEY_B) apply(Action::Back);
	if (down & KEY_X) apply(Action::Search);
	if (down & KEY_Y) apply(Action::Favorite);
	if (down & KEY_L) apply(Action::PrevTab);
	if (down & KEY_R) apply(Action::NextTab);
	if (down & KEY_SELECT) apply(Action::ToggleView);
	if (down & KEY_START) apply(Action::Menu);
	if (down & KEY_TOUCH) {
		touchPosition touch;
		touchRead(&touch);
		apply(Action::Tap, touch.px, touch.py);
	}
	return launch;
}

// Loads the selected game's box art after the cursor has rested for a few frames, remembering games
// that have none so their file is not looked up again.
class CoverLoader {
public:
	void update(App& app) {
		const GameEntry* game = app.selected();
		const std::string path = game ? game->path : std::string();
		if (path != pending_) {
			pending_ = path;
			restingFrames_ = 0;
			return;
		}
		if (path.empty() || path == loaded_ || ++restingFrames_ < kCoverDelayFrames) return;
		loaded_ = path;
		std::optional<Cover> cover;
		if (!missing_.count(path)) {
			cover = storage::loadCover(path);
			if (!cover) missing_.insert(path);
		}
		app.setCover(path, std::move(cover));
	}

private:
	std::string pending_;
	std::string loaded_;
	int restingFrames_ = 0;
	std::set<std::string> missing_;
};

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

void launch(Screens& screens, UserData& userData, Config& config, const std::string& path) {
	userData.recordLaunch(path, uint32_t(time(nullptr)));
	storage::saveUserData(userData);
	config.selectedPath = path;
	storage::saveConfig(config);

	fadeOut(kFadeFrames);
	int code = 0;
	const LaunchError error = launchViaTwilight(path, &code);
	setBrightness(3, 0);
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

	// main.srldr fades both screens to white before booting its theme and leaves the fade-in to it:
	// start from black and fade in once there is something to show.
	setBrightness(3, -16);
	Screens screens;
	if (!sys().fatInitOk()) {
		setBrightness(3, 0);
		halt(screens, "SD card not found", {"DSCore needs the DSi SD card."});
	}
	showMessage(screens, "DSCore", {"Loading..."}, "", {});
	fadeIn(kFadeFrames);
	storage::ensureDataDir();

	std::string log = std::string("DSCore ") + kVersion + " boot\n";
	Config config = storage::loadConfig();
	const std::vector<Theme> themes = storage::loadThemes();
	messageTheme = &findTheme(themes, config.theme);
	UserData userData = storage::loadUserData();
	LibraryData library = loadLibrary(screens, log);
	App app(library, userData, config);
	app.setThemes(themes);
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
	CoverLoader covers;
	SoundEffects sounds;
	BottomFade bottomFade;
	while (true) {
		if (powerButtonPressed()) {
			if (configSaveCountdown > 0) storage::saveConfig(config);
			returnToSystemMenu();
		}
		const std::string path = handleInput(app);
		const Sound sound = app.takeSound();
		if (config.sound) sounds.play(sound);
		if (!path.empty()) {
			storage::appendLog(drawStats.summary());
			launch(screens, userData, config, path);
			configSaveCountdown = -1;
			app.invalidate();
			continue;
		}
		if (app.takeRebuildRequest()) {
			std::string rebuildLog = "rebuild\n";
			library = loadLibrary(screens, rebuildLog, true);
			storage::appendLog(rebuildLog);
			app.libraryChanged();
		}
		messageTheme = &app.theme();
		covers.update(app);
		if (app.takeUserDataChanged()) storage::saveUserData(userData);
		if (app.takeConfigChanged()) configSaveCountdown = kConfigSaveDelayFrames;
		if (configSaveCountdown > 0 && --configSaveCountdown == 0) storage::saveConfig(config);

		if (app.takeRedraw()) {
			const unsigned start = elapsedMs();
			app.drawTop(screens.top());
			app.drawBottom(screens.bottom());
			drawStats.add(elapsedMs() - start);
			screens.present();
			if (app.takeBottomTransition()) bottomFade.start();
		} else {
			swiWaitForVBlank();
		}
		bottomFade.update();
	}
}
