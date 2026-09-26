#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dscore {

bool fileExists(const std::string& path);
bool readFile(const std::string& path, std::string& out);
bool readFile(const std::string& path, std::vector<uint8_t>& out);
bool writeFile(const std::string& path, const void* data, size_t size);

// Writes data to path + ".tmp" first so a failed write never truncates path, then swaps it in.
bool replaceFile(const std::string& path, const void* data, size_t size);

// Creates path and its missing parents; true when the directory exists afterwards.
bool makeDirectories(const std::string& path);

} // namespace dscore
