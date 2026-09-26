#include <nds.h>
#include <dirent.h>
#include <cstdio>
#include <string>
#include <vector>

#include "common/systemdetails.h"
#include "core/NdsHeader.h"
#include "core/Text.h"
#include "launch/TwilightLauncher.h"
#include "my_gurumeditation.h"

// Read by TWiLight's twlmenusettings.cpp; normally defined in universal/arm9/source/mainAll.cpp.
bool useTwlCfg = false;

namespace {

constexpr const char* kRomRoot = "sd:/roms/NDS";
constexpr size_t kMaxRoms = 50;
constexpr int kMaxDepth = 4;
constexpr size_t kVisibleRows = 22;
constexpr size_t kNameColumns = 30;

PrintConsole topScreen;
PrintConsole bottomScreen;

void scanDir(const std::string& dir, int depth, std::vector<std::string>& out) {
	if (depth > kMaxDepth || out.size() >= kMaxRoms) return;
	DIR* d = opendir(dir.c_str());
	if (!d) return;
	while (dirent* entry = readdir(d)) {
		if (out.size() >= kMaxRoms) break;
		if (entry->d_name[0] == '.') continue;
		const std::string path = dir + "/" + entry->d_name;
		if (entry->d_type == DT_DIR) {
			scanDir(path, depth + 1, out);
		} else if (dscore::hasExtension(entry->d_name, ".nds")) {
			out.push_back(path);
		}
	}
	closedir(d);
}

std::string readTitle(const std::string& path) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return "(cannot open file)";
	std::string title;
	uint8_t header[dscore::kNdsHeaderSize];
	dscore::NdsHeaderInfo info;
	if (fread(header, 1, sizeof(header), f) == sizeof(header)
		&& dscore::parseNdsHeader(header, sizeof(header), info) && info.bannerOffset != 0) {
		static uint8_t banner[dscore::kBannerTitlesEnd];
		if (fseek(f, info.bannerOffset, SEEK_SET) == 0 && fread(banner, 1, sizeof(banner), f) == sizeof(banner)) {
			title = dscore::bannerTitle(banner, sizeof(banner), dscore::BannerLanguage::English);
		}
	}
	fclose(f);
	return title.empty() ? "(no banner title)" : title;
}

std::string fileName(const std::string& path) {
	const size_t slash = path.rfind('/');
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

void drawTop(const std::string& path, const std::string& title, unsigned scanMs, size_t count) {
	consoleSelect(&topScreen);
	consoleClear();
	iprintf("DSCore - Milestone 1\n\n");
	iprintf("DSi mode: %s\n", isDSiMode() ? "yes" : "no");
	iprintf("Scan: %u ms (%u ROMs)\n\n", scanMs, unsigned(count));
	if (path.empty()) {
		iprintf("No .nds found in %s\n", kRomRoot);
		return;
	}
	iprintf("Title:\n%s\n\n", dscore::asciiForConsole(title).c_str());
	iprintf("File:\n%s\n\n", dscore::asciiForConsole(path).c_str());
	iprintf("A: play   Up/Down: move\n");
}

void drawList(const std::vector<std::string>& roms, size_t cursor) {
	consoleSelect(&bottomScreen);
	consoleClear();
	iprintf("%s (%u)\n", kRomRoot, unsigned(roms.size()));
	size_t first = cursor > kVisibleRows / 2 ? cursor - kVisibleRows / 2 : 0;
	if (first + kVisibleRows > roms.size()) first = roms.size() > kVisibleRows ? roms.size() - kVisibleRows : 0;
	for (size_t i = first; i < roms.size() && i < first + kVisibleRows; ++i) {
		std::string name = dscore::asciiForConsole(fileName(roms[i]));
		if (name.size() > kNameColumns) name.resize(kNameColumns);
		iprintf("%c%s\n", i == cursor ? '>' : ' ', name.c_str());
	}
}

void waitForB() {
	do {
		swiWaitForVBlank();
		scanKeys();
	} while (!(keysDown() & KEY_B));
}

} // namespace

int main(int argc, char** argv) {
	myExceptionHandler();
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);
	sys().initFilesystem(argc > 0 ? argv[0] : "sd:/dscore.nds");
	sys().initArm7RegStatuses();

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);
	consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	if (!sys().fatInitOk()) {
		consoleSelect(&topScreen);
		iprintf("FAT init failed\n");
		while (true) swiWaitForVBlank();
	}

	std::vector<std::string> roms;
	cpuStartTiming(0);
	scanDir(kRomRoot, 0, roms);
	const unsigned scanMs = timerTicks2msec(cpuEndTiming());

	size_t cursor = 0;
	auto refresh = [&]() {
		drawList(roms, cursor);
		const std::string path = roms.empty() ? std::string() : roms[cursor];
		drawTop(path, path.empty() ? std::string() : readTitle(path), scanMs, roms.size());
	};
	refresh();

	keysSetRepeat(15, 4);
	while (true) {
		swiWaitForVBlank();
		scanKeys();
		if (roms.empty()) continue;
		const u32 repeat = keysDownRepeat();
		if ((repeat & KEY_DOWN) && cursor + 1 < roms.size()) {
			++cursor;
			refresh();
		} else if ((repeat & KEY_UP) && cursor > 0) {
			--cursor;
			refresh();
		} else if (keysDown() & KEY_A) {
			consoleSelect(&topScreen);
			iprintf("\nLaunching...\n");
			int code = 0;
			const dscore::LaunchError err = dscore::launchViaTwilight(roms[cursor], &code);
			iprintf("Failed: %s (code %d)\nB: back\n", dscore::describe(err), code);
			waitForB();
			refresh();
		}
	}
}
