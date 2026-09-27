#pragma once

#include "core/Config.h"
#include "core/LibraryCache.h"
#include "core/UserData.h"

namespace dscore::storage {

// DSCore's own files on the SD card; TWiLight Menu++'s files are only read.
constexpr const char* kDataDir = "sd:/_nds/DSCore";

bool ensureDataDir();

// False when there is no usable cache (missing, damaged or from another format version).
bool loadLibrary(LibraryData& library);
bool saveLibrary(const LibraryData& library);

// Loads userdata.ini; on the first run (no file yet) seeds it from TWiLight Menu++'s play history.
UserData loadUserData();
bool saveUserData(const UserData& userData);

Config loadConfig();
bool saveConfig(const Config& config);

// Overwrites the log with text (timings for tuning on real hardware); appendLog adds to it.
void writeBootLog(const std::string& text);
void appendLog(const std::string& text);

} // namespace dscore::storage
