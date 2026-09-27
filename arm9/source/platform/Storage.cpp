#include "platform/Storage.h"

#include <string>
#include <vector>

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

std::optional<Cover> loadCover(const std::string& romPath) {
	std::vector<uint8_t> bytes;
	if (!readFile(std::string(kDataDir) + "/covers/" + coverFileName(romPath), bytes)) return std::nullopt;
	return decodeCover(bytes.data(), bytes.size());
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
