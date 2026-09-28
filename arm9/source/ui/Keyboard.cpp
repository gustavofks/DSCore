#include "ui/Keyboard.h"

#include <cstdlib>

namespace dscore {

namespace {

constexpr int kColumns = 10;
constexpr int kKeyW = 25;
constexpr int kKeyH = 26;
constexpr int kLeft = 3;
constexpr int kTop = 24;
constexpr int kGap = 3;

std::vector<Key> buildKeys() {
	static const char* const kRows[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL'", "ZXCVBNM-.&"};
	static char labels[4][kColumns][2];
	std::vector<Key> keys;
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < kColumns; ++col) {
			labels[row][col][0] = kRows[row][col];
			labels[row][col][1] = '\0';
			const Rect rect = {kLeft + col * kKeyW, kTop + row * (kKeyH + kGap), kKeyW - kGap, kKeyH};
			keys.push_back({labels[row][col], kRows[row][col], KeyKind::Char, rect, row});
		}
	}
	const int y = kTop + 4 * (kKeyH + kGap);
	keys.push_back({"Space", ' ', KeyKind::Space, {kLeft, y, 5 * kKeyW - kGap, kKeyH}, 4});
	keys.push_back({"Del", '\b', KeyKind::Delete, {kLeft + 5 * kKeyW, y, 2 * kKeyW - kGap, kKeyH}, 4});
	keys.push_back({"OK", '\n', KeyKind::Done, {kLeft + 7 * kKeyW, y, 3 * kKeyW - kGap, kKeyH}, 4});
	return keys;
}

int centerX(const Key& key) {
	return key.rect.x + key.rect.w / 2;
}

} // namespace

const std::vector<Key>& keyboardKeys() {
	static const std::vector<Key> keys = buildKeys();
	return keys;
}

int keyIndexFor(char value) {
	const std::vector<Key>& keys = keyboardKeys();
	for (size_t i = 0; i < keys.size(); ++i) {
		if (keys[i].value == value) return int(i);
	}
	return -1;
}

int moveKey(int index, Move move) {
	const std::vector<Key>& keys = keyboardKeys();
	const Key& from = keys[size_t(index)];
	if (move == Move::Left || move == Move::Right) {
		const int next = index + (move == Move::Right ? 1 : -1);
		if (next < 0 || next >= int(keys.size()) || keys[size_t(next)].row != from.row) return index;
		return next;
	}
	const int row = from.row + (move == Move::Down ? 1 : -1);
	int best = index;
	int bestDistance = 1 << 30;
	for (size_t i = 0; i < keys.size(); ++i) {
		if (keys[i].row != row) continue;
		const int distance = std::abs(centerX(keys[i]) - centerX(from));
		if (distance < bestDistance) {
			best = int(i);
			bestDistance = distance;
		}
	}
	return best;
}

int keyAt(int x, int y) {
	const std::vector<Key>& keys = keyboardKeys();
	for (size_t i = 0; i < keys.size(); ++i) {
		if (keys[i].rect.contains(x, y)) return int(i);
	}
	return -1;
}

KeyResult applyKey(const Key& key, std::string& query) {
	switch (key.kind) {
		case KeyKind::Done: return KeyResult::Done;
		case KeyKind::Delete:
			if (query.empty()) return KeyResult::None;
			query.pop_back();
			return KeyResult::Edited;
		case KeyKind::Space:
		case KeyKind::Char:
			if (query.size() >= kMaxQueryLength) return KeyResult::None;
			query += key.value;
			return KeyResult::Edited;
	}
	return KeyResult::None;
}

} // namespace dscore
