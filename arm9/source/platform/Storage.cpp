#include "platform/Storage.h"

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <string>
#include <vector>

#include "core/Text.h"
#include "platform/FileIo.h"

namespace dscore::storage {

namespace {

const std::string kLibraryPath = std::string(kDataDir) + "/library.bin";
const std::string kUserDataPath = std::string(kDataDir) + "/userdata.ini";
const std::string kConfigPath = std::string(kDataDir) + "/config.ini";
const std::string kBootLogPath = std::string(kDataDir) + "/log.txt";
constexpr const char* kTwilightExtras = "sd:/_nds/TWiLightMenu/extras";

bool saveText(const std::string& path, const std::string& text) {
	return replaceFile(path, text.data(), text.size());
}

} // namespace

bool ensureDataDir() {
	return makeDirectories(kDataDir);
}

bool loadLibrary(LibraryData& library) {
	std::vector<uint8_t> bytes;
	return readFile(kLibraryPath, bytes) && decodeLibrary(bytes.data(), bytes.size(), library);
}

bool saveLibrary(const LibraryData& library) {
	const std::vector<uint8_t> bytes = encodeLibrary(library);
	return replaceFile(kLibraryPath, bytes.data(), bytes.size());
}

UserData loadUserData() {
	std::string text;
	if (readFile(kUserDataPath, text)) return UserData::parse(text);

	UserData imported;
	std::string recent, times;
	readFile(std::string(kTwilightExtras) + "/recentlyplayed.ini", recent);
	readFile(std::string(kTwilightExtras) + "/timesplayed.ini", times);
	imported.importTwilightHistory(recent, times);
	saveUserData(imported); // written once so the import never runs again
	return imported;
}

bool saveUserData(const UserData& userData) {
	return saveText(kUserDataPath, userData.serialize());
}

namespace {

FILE* thumbsFile = nullptr;

} // namespace

bool openThumbs(std::vector<ThumbEntry>& entries) {
	if (thumbsFile) fclose(thumbsFile);
	thumbsFile = fopen((std::string(kDataDir) + "/thumbs.bin").c_str(), "rb");
	if (!thumbsFile) return false;
	uint8_t header[kThumbHeaderSize];
	uint32_t count = 0;
	if (fread(header, 1, sizeof(header), thumbsFile) != sizeof(header) || !parseThumbHeader(header, sizeof(header), count)) {
		return false;
	}
	std::vector<uint8_t> index(size_t(count) * kThumbEntrySize);
	return fread(index.data(), 1, index.size(), thumbsFile) == index.size()
		&& parseThumbEntries(index.data(), index.size(), count, entries);
}

bool readThumb(const ThumbEntry& entry, uint16_t* out) {
	const size_t bytes = size_t(entry.width) * entry.height * 2;
	// DS colors are stored little-endian, the ARM9's own byte order.
	return thumbsFile && fseek(thumbsFile, long(entry.offset), SEEK_SET) == 0 && fread(out, 1, bytes, thumbsFile) == bytes;
}

MetadataMap loadMetadata() {
	std::string ini;
	if (!readFile(std::string(kDataDir) + "/metadata.ini", ini)) return {};
	return parseMetadata(ini);
}

std::optional<Cover> loadCover(const std::string& romPath) {
	std::vector<uint8_t> bytes;
	if (!readFile(std::string(kDataDir) + "/covers/" + coverFileName(romPath), bytes)) return std::nullopt;
	return decodeCover(bytes.data(), bytes.size());
}

std::vector<Theme> loadThemes() {
	std::vector<Theme> themes = builtInThemes();
	const std::string dir = std::string(kDataDir) + "/themes";
	std::vector<std::string> files;
	if (DIR* d = opendir(dir.c_str())) {
		while (dirent* entry = readdir(d)) {
			if (entry->d_name[0] != '.' && hasExtension(entry->d_name, ".ini")) files.push_back(entry->d_name);
		}
		closedir(d);
	}
	std::sort(files.begin(), files.end());
	for (const std::string& file : files) {
		std::string text;
		if (!readFile(dir + "/" + file, text)) continue;
		themes.push_back(parseTheme(text, themes[0], file.substr(0, file.size() - 4)));
	}
	return themes;
}

Config loadConfig() {
	std::string text;
	return readFile(kConfigPath, text) ? Config::parse(text) : Config{};
}

bool saveConfig(const Config& config) {
	return saveText(kConfigPath, config.serialize());
}

void writeBootLog(const std::string& text) {
	writeFile(kBootLogPath, text.data(), text.size());
}

void appendLog(const std::string& text) {
	std::string log;
	readFile(kBootLogPath, log);
	log += text;
	writeFile(kBootLogPath, log.data(), log.size());
}

} // namespace dscore::storage
