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

// Bottom screen: tab bar, then the grid or the list, then a footer.
constexpr int kTabBarH = 20;
constexpr int kTabPadding = 12; // around a label that sets its tab width
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

// Tab bar for labels of the given widths in pixels. Tabs share the screen width evenly when every label
// fits that; otherwise each gets its label width plus padding, and when even that is too wide the bar
// scrolls to keep the active tab in view (rects may then lie partly or wholly off screen).
std::vector<Rect> tabRects(const std::vector<int>& labelWidths, int active);
Rect menuRowRect(int row);
Rect footerFilterRect(); // footer labels that cycle the filter and the sort order when touched
Rect footerSortRect();
Rect gridCellRect(int slot); // slot 0..kGridPerPage-1 on the current page
Rect listRowRect(int row);

// Hit tests; -1 when (x, y) is outside every target.
int tabAt(const std::vector<Rect>& tabs, int x, int y);
int gridSlotAt(int x, int y);
int listRowAt(int x, int y);
int menuRowAt(int x, int y, int rows);

} // namespace layout

} // namespace dscore
