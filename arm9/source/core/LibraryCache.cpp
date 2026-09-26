#include "core/LibraryCache.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace dscore {

namespace {

constexpr char kMagic[4] = {'D', 'S', 'C', 'L'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderSize = 20; // magic, version, reserved, game count, icon count, checksum
constexpr size_t kIconBytes = 512 + 16 * 2;

uint32_t fnv1a(const uint8_t* data, size_t len) {
	uint32_t hash = 2166136261u;
	for (size_t i = 0; i < len; ++i) {
		hash ^= data[i];
		hash *= 16777619u;
	}
	return hash;
}

class Writer {
public:
	explicit Writer(std::vector<uint8_t>& out) : out_(out) {}
	void u8(uint8_t v) { out_.push_back(v); }
	void u16(uint16_t v) { u8(uint8_t(v)); u8(uint8_t(v >> 8)); }
	void u32(uint32_t v) { u16(uint16_t(v)); u16(uint16_t(v >> 16)); }
	void bytes(const void* data, size_t len) {
		const auto* p = static_cast<const uint8_t*>(data);
		out_.insert(out_.end(), p, p + len);
	}
	void str(const std::string& s) {
		u16(uint16_t(s.size()));
		bytes(s.data(), s.size());
	}

private:
	std::vector<uint8_t>& out_;
};

class Reader {
public:
	Reader(const uint8_t* data, size_t len) : data_(data), len_(len) {}
	bool ok() const { return ok_; }
	bool atEnd() const { return pos_ == len_; }
	uint8_t u8() { return take(1) ? data_[pos_ - 1] : 0; }
	uint16_t u16() { const uint16_t lo = u8(); return uint16_t(lo | (u8() << 8)); }
	uint32_t u32() { const uint32_t lo = u16(); return lo | (uint32_t(u16()) << 16); }
	void bytes(void* out, size_t len) {
		if (take(len)) std::memcpy(out, data_ + pos_ - len, len);
	}
	std::string str() {
		const uint16_t len = u16();
		if (!take(len)) return {};
		return std::string(reinterpret_cast<const char*>(data_ + pos_ - len), len);
	}

private:
	bool take(size_t n) {
		if (!ok_ || len_ - pos_ < n) {
			ok_ = false;
			return false;
		}
		pos_ += n;
		return true;
	}

	const uint8_t* data_;
	size_t len_;
	size_t pos_ = 0;
	bool ok_ = true;
};

} // namespace

std::vector<uint8_t> encodeLibrary(const LibraryData& library) {
	std::vector<uint8_t> payload;
	Writer w(payload);
	for (const GameEntry& game : library.games) {
		w.u8(uint8_t(game.system));
		w.u32(game.fileSize);
		w.u32(uint32_t(game.iconIndex));
		w.str(game.path);
		w.str(game.title);
		char code[4] = {0, 0, 0, 0};
		std::memcpy(code, game.gameCode.data(), std::min<size_t>(4, game.gameCode.size()));
		w.bytes(code, sizeof(code));
	}
	for (const NdsIcon& icon : library.icons) {
		w.bytes(icon.bitmap, sizeof(icon.bitmap));
		for (uint16_t color : icon.palette) w.u16(color);
	}

	std::vector<uint8_t> out;
	out.reserve(kHeaderSize + payload.size());
	Writer h(out);
	h.bytes(kMagic, sizeof(kMagic));
	h.u16(kVersion);
	h.u16(0);
	h.u32(uint32_t(library.games.size()));
	h.u32(uint32_t(library.icons.size()));
	h.u32(fnv1a(payload.data(), payload.size()));
	h.bytes(payload.data(), payload.size());
	return out;
}

bool decodeLibrary(const uint8_t* data, size_t len, LibraryData& out) {
	if (len < kHeaderSize || std::memcmp(data, kMagic, sizeof(kMagic)) != 0) return false;
	Reader header(data + sizeof(kMagic), kHeaderSize - sizeof(kMagic));
	const uint16_t version = header.u16();
	header.u16();
	const uint32_t gameCount = header.u32();
	const uint32_t iconCount = header.u32();
	const uint32_t checksum = header.u32();
	if (version != kVersion || fnv1a(data + kHeaderSize, len - kHeaderSize) != checksum) return false;
	if (iconCount > (len - kHeaderSize) / kIconBytes) return false;

	LibraryData library;
	Reader r(data + kHeaderSize, len - kHeaderSize);
	for (uint32_t i = 0; i < gameCount && r.ok(); ++i) {
		GameEntry game;
		game.system = System(r.u8());
		game.fileSize = r.u32();
		game.iconIndex = int32_t(r.u32());
		game.path = r.str();
		game.title = r.str();
		char code[4];
		r.bytes(code, sizeof(code));
		game.gameCode.assign(code, strnlen(code, sizeof(code)));
		if (game.system != System::Nds && game.system != System::Gba) return false;
		if (game.iconIndex < -1 || game.iconIndex >= int32_t(iconCount)) return false;
		library.games.push_back(std::move(game));
	}
	library.icons.resize(iconCount);
	for (NdsIcon& icon : library.icons) {
		r.bytes(icon.bitmap, sizeof(icon.bitmap));
		for (uint16_t& color : icon.palette) color = r.u16();
	}
	if (!r.ok() || !r.atEnd()) return false;

	out = std::move(library);
	return true;
}

} // namespace dscore
