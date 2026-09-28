#pragma once

#include <cstddef>

#include "core/Library.h"
#include "gfx/Canvas.h"

namespace dscore {

// Screen geometry shared by drawing and touch hit-testing. Both screens are 256x192.
namespace layout {

constexpr int kScreenW = 256;
constexpr int kScreenH = 192;

// Bottom screen: tab bar, then the grid or the list, then a footer.
constexpr int kTabBarH = 20;
constexpr int kTabCount = 5;
constexpr int kFooterH = 16;
constexpr int kContentY = kTabBarH + 2;
constexpr int kContentH = kScreenH - kFooterH - kContentY;

constexpr int kGridCols = 5;
constexpr int kGridRows = 3;
constexpr int kCellW = kScreenW / kGridCols; // 51
constexpr int kCellH = kContentH / kGridRows;
constexpr int kGridPerPage = kGridCols * kGridRows;

constexpr int kListRowH = 16;
constexpr int kListRows = kContentH / kListRowH;

// Options menu (bottom screen).
constexpr int kMenuTop = 28;
constexpr int kMenuRowH = 24;

Rect tabRect(int index);
Rect menuRowRect(int row);
Rect gridCellRect(int slot); // slot 0..kGridPerPage-1 on the current page
Rect listRowRect(int row);

// Hit tests; -1 when (x, y) is outside every target.
int tabAt(int x, int y);
int gridSlotAt(int x, int y);
int listRowAt(int x, int y);
int menuRowAt(int x, int y, int rows);

} // namespace layout

} // namespace dscore
