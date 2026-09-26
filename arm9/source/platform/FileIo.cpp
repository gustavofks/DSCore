#include "platform/FileIo.h"

#include <cstdint>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

namespace dscore {

namespace {

template <typename Buffer>
bool readInto(const std::string& path, Buffer& out) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return false;
	out.clear();
	char chunk[4096];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) out.insert(out.end(), chunk, chunk + n);
	const bool ok = !ferror(f);
	fclose(f);
	return ok;
}

} // namespace

bool fileExists(const std::string& path) {
	return access(path.c_str(), F_OK) == 0;
}

bool readFile(const std::string& path, std::string& out) {
	return readInto(path, out);
}

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
	return readInto(path, out);
}

bool writeFile(const std::string& path, const void* data, size_t size) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) return false;
	const bool written = size == 0 || fwrite(data, 1, size, f) == size;
	const bool closed = fclose(f) == 0;
	return written && closed;
}

bool replaceFile(const std::string& path, const void* data, size_t size) {
	const std::string temp = path + ".tmp";
	if (!writeFile(temp, data, size)) return false;
	remove(path.c_str());
	if (rename(temp.c_str(), path.c_str()) == 0) return true;
	// FAT rename can fail after the remove; fall back to writing the file directly.
	const bool ok = writeFile(path, data, size);
	remove(temp.c_str());
	return ok;
}

bool makeDirectories(const std::string& path) {
	const size_t root = path.find(":/");
	size_t pos = root == std::string::npos ? 1 : root + 2; // skip the drive prefix, e.g. "sd:/"
	while ((pos = path.find('/', pos)) != std::string::npos) {
		mkdir(path.substr(0, pos).c_str(), 0777);
		++pos;
	}
	mkdir(path.c_str(), 0777);
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

} // namespace dscore
