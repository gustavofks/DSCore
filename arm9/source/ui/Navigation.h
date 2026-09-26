#pragma once

#include <cstddef>

#include "core/Config.h"

namespace dscore {

enum class Move { Left, Right, Up, Down };

// New cursor position in a view of count items. Grid: left/right step one item, up/down one row.
// List: up/down step one row, left/right one page. The cursor never leaves [0, count).
size_t moveCursor(size_t cursor, size_t count, ViewMode view, Move move);

// Items per page and the first item of the page that holds cursor.
size_t pageSize(ViewMode view);
size_t pageStart(size_t cursor, ViewMode view);

} // namespace dscore
