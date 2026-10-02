#pragma once

#include <cstddef>
#include <vector>

#include "core/Library.h"
#include "gfx/Canvas.h"

namespace dscore {

// Screen geometry shared by drawing and touch hit-testing. Both screens are 256x192.
namespace layout {

constexpr int kScreenW = 256;
constexpr int kScreenH = 192;

// Bottom screen: a row of tab chips, then the cards or the list, then a footer.
constexpr int kTabBarH = 24;
constexpr int kChipY = 4;
constexpr int kChipH = 16;
constexpr int kChipPadding = 14; // around a chip's label
constexpr int kChipGap = 5;
constexpr int kChipMargin = 6;
constexpr int kFooterH = 16;
constexpr int kContentY = kTabBarH + 2;
constexpr int kContentH = kScreenH - kFooterH - kContentY;

constexpr int kGridCols = 4;
constexpr int kGridRows = 3;
constexpr int kGridMarginX = 4;
constexpr int kCellW = (kScreenW - 2 * kGridMarginX) / kGridCols; // 62
constexpr int kCellH = kContentH / kGridRows;                     // 50
constexpr int kGridPerPage = kGridCols * kGridRows;

constexpr int kListRowH = 16;
constexpr int kListRows = kContentH / kListRowH;

// Options menu (bottom screen).
constexpr int kMenuTop = 24;
constexpr int kMenuRowH = 20;
constexpr int kMenuVisibleRows = (kScreenH - kFooterH - kMenuTop) / kMenuRowH;

// Tab chips for labels of the given widths in pixels, left to right. When they do not fit, the row
// scrolls to keep the active chip in view (rects may then lie partly or wholly off screen).
std::vector<Rect> chipRects(const std::vector<int>& labelWidths, int active);
// First menu row on screen: the list scrolls to keep selected in view.
int menuFirstRow(int selected, int rows);
Rect menuRowRect(int slot); // slot 0..kMenuVisibleRows-1 on screen
Rect footerFilterRect(); // footer labels that cycle the filter and the sort order when touched
Rect footerSortRect();
Rect gridCellRect(int slot); // slot 0..kGridPerPage-1 on the current page
Rect listRowRect(int row);

// Hit tests; -1 when (x, y) is outside every target.
int tabAt(const std::vector<Rect>& tabs, int x, int y);
int gridSlotAt(int x, int y);
int listRowAt(int x, int y);
int menuRowAt(int x, int y, int selected, int rows); // menu row under (x, y) with the list scrolled for selected

} // namespace layout

} // namespace dscore
