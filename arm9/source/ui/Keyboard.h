#pragma once

#include <string>
#include <vector>

#include "gfx/Canvas.h"
#include "ui/Navigation.h"

namespace dscore {

// On-screen keyboard for the bottom screen: digits, QWERTY letters, a few symbols, space, delete and OK.
enum class KeyKind { Char, Space, Delete, Done };

struct Key {
	const char* label;
	char value; // the typed character; ' ' for space, '\b' for delete, '\n' for OK
	KeyKind kind;
	Rect rect;
	int row;
};

constexpr size_t kMaxQueryLength = 24;

const std::vector<Key>& keyboardKeys();

// Index of the key typing value ('\b' = delete, '\n' = OK); -1 if there is none.
int keyIndexFor(char value);

// Key reached from index by the D-pad: left/right within the row, up/down to the key of the next row
// closest to the same column. Stays put at the edges.
int moveKey(int index, Move move);

// Key under the touch point; -1 outside the keyboard.
int keyAt(int x, int y);

enum class KeyResult { None, Edited, Done };

// Applies a key press to query (at most kMaxQueryLength characters).
KeyResult applyKey(const Key& key, std::string& query);

} // namespace dscore
