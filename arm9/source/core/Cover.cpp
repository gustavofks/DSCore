#include "core/Cover.h"

#include <cstring>

namespace dscore {

namespace {

constexpr size_t kHeaderSize = 8;

} // namespace

std::optional<Cover> decodeCover(const uint8_t* data, size_t len) {
	if (len < kHeaderSize || std::memcmp(data, "DSCV", 4) != 0) return std::nullopt;
	Cover cover;
	cover.width = data[4] | (data[5] << 8);
	cover.height = data[6] | (data[7] << 8);
	if (cover.width <= 0 || cover.height <= 0 || cover.width > kMaxCoverSize || cover.height > kMaxCoverSize) {
		return std::nullopt;
	}
	const size_t count = size_t(cover.width) * size_t(cover.height);
	if (len != kHeaderSize + count * 2) return std::nullopt;
	cover.pixels.resize(count);
	for (size_t i = 0; i < count; ++i) {
		cover.pixels[i] = uint16_t(data[kHeaderSize + 2 * i] | (data[kHeaderSize + 2 * i + 1] << 8));
	}
	return cover;
}

std::string coverFileName(const std::string& romPath) {
	const size_t slash = romPath.find_last_of('/');
	return (slash == std::string::npos ? romPath : romPath.substr(slash + 1)) + ".bin";
}

} // namespace dscore
