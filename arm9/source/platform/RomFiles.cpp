#include "platform/RomFiles.h"

#include <cstdio>
#include <dirent.h>

#include "core/Systems.h"
#include "core/Titles.h"

namespace dscore {

namespace {

constexpr int kMaxDepth = 8;

void scanFolder(const std::string& dir, int depth, std::vector<std::string>& out) {
	if (depth > kMaxDepth) return;
	DIR* d = opendir(dir.c_str());
	if (!d) return;
	const std::string prefix = (!dir.empty() && dir.back() == '/') ? dir : dir + "/";
	while (dirent* entry = readdir(d)) {
		if (entry->d_name[0] == '.') continue;
		const std::string path = prefix + entry->d_name;
		if (entry->d_type == DT_DIR) {
			scanFolder(path, depth + 1, out);
		} else if (System system; systemForPath(path, system)) {
			out.push_back(path);
		}
	}
	closedir(d);
}

bool readAt(FILE* f, long offset, void* out, size_t size) {
	return fseek(f, offset, SEEK_SET) == 0 && fread(out, 1, size, f) == size;
}

uint32_t fileSize(FILE* f) {
	if (fseek(f, 0, SEEK_END) != 0) return 0;
	const long size = ftell(f);
	return size > 0 ? uint32_t(size) : 0;
}

bool readNds(FILE* f, const std::string& path, BannerLanguage lang, GameEntry& game, std::optional<NdsIcon>& icon) {
	uint8_t header[kNdsHeaderSize];
	NdsHeaderInfo info;
	if (!readAt(f, 0, header, sizeof(header)) || !parseNdsHeader(header, sizeof(header), info)) return false;
	game.system = System::Nds;
	game.gameCode = info.gameCode;

	uint8_t banner[kBannerTitlesEnd];
	const bool hasBanner = info.bannerOffset != 0 && readAt(f, long(info.bannerOffset), banner, sizeof(banner));
	std::string text;
	if (hasBanner) {
		text = bannerText(banner, sizeof(banner), lang);
		if (text.empty()) text = bannerText(banner, sizeof(banner), BannerLanguage::English);
	}
	const BannerText parsed = parseBannerText(text, titleFromFileName(path));
	game.title = parsed.title;
	game.publisher = parsed.publisher;
	NdsIcon bannerIcon;
	if (hasBanner && readNdsIcon(banner, sizeof(banner), bannerIcon)) icon = bannerIcon;
	return true;
}

bool readGba(FILE* f, const std::string& path, GameEntry& game) {
	uint8_t header[kGbaHeaderEnd];
	GbaHeaderInfo info;
	game.system = System::Gba;
	game.title = titleFromFileName(path);
	if (readAt(f, 0, header, sizeof(header)) && parseGbaHeader(header, sizeof(header), info)) game.gameCode = info.gameCode;
	return true;
}

} // namespace

std::vector<std::string> listRomFiles(const std::vector<std::string>& roots) {
	std::vector<std::string> files;
	for (const std::string& root : roots) scanFolder(root, 0, files);
	return files;
}

bool readRomInfo(const std::string& path, BannerLanguage lang, GameEntry& game, std::optional<NdsIcon>& icon) {
	System system;
	if (!systemForPath(path, system)) return false;
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return false;
	bool ok = false;
	switch (system) {
		case System::Nds: ok = readNds(f, path, lang, game, icon); break;
		case System::Gba: ok = readGba(f, path, game); break;
		default:
			// Emulated consoles: the No-Intro file name is a better title than the cartridge header.
			game.system = system;
			game.title = titleFromFileName(path);
			ok = true;
			break;
	}
	if (ok) {
		game.fileSize = fileSize(f);
		game.portuguese = parseFileTags(path).portuguese;
	}
	fclose(f);
	return ok;
}

} // namespace dscore
