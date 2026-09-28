#include "ui/Layout.h"

namespace dscore::layout {

Rect tabRect(int index) {
	const int x0 = index * kScreenW / kTabCount;
	const int x1 = (index + 1) * kScreenW / kTabCount;
	return {x0, 0, x1 - x0, kTabBarH};
}

Rect gridCellRect(int slot) {
	const int col = slot % kGridCols;
	const int row = slot / kGridCols;
	const int marginX = (kScreenW - kGridCols * kCellW) / 2;
	return {marginX + col * kCellW, kContentY + row * kCellH, kCellW, kCellH};
}

Rect listRowRect(int row) {
	return {0, kContentY + row * kListRowH, kScreenW, kListRowH};
}

Rect menuRowRect(int row) {
	return {12, kMenuTop + row * kMenuRowH, kScreenW - 24, kMenuRowH - 4};
}

int menuRowAt(int x, int y, int rows) {
	for (int row = 0; row < rows; ++row) {
		if (menuRowRect(row).contains(x, y)) return row;
	}
	return -1;
}

int tabAt(int x, int y) {
	for (int i = 0; i < kTabCount; ++i) {
		if (tabRect(i).contains(x, y)) return i;
	}
	return -1;
}

int gridSlotAt(int x, int y) {
	for (int slot = 0; slot < kGridPerPage; ++slot) {
		if (gridCellRect(slot).contains(x, y)) return slot;
	}
	return -1;
}

int listRowAt(int x, int y) {
	for (int row = 0; row < kListRows; ++row) {
		if (listRowRect(row).contains(x, y)) return row;
	}
	return -1;
}

} // namespace dscore::layout
