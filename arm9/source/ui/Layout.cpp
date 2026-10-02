#include "ui/Layout.h"

#include <algorithm>

namespace dscore::layout {

std::vector<Rect> chipRects(const std::vector<int>& labelWidths, int active) {
	std::vector<Rect> rects;
	int x = kChipMargin;
	for (int w : labelWidths) {
		rects.push_back({x, kChipY, w + kChipPadding, kChipH});
		x += w + kChipPadding + kChipGap;
	}
	const int total = x - kChipGap + kChipMargin;
	if (total > kScreenW && active >= 0 && active < int(rects.size())) {
		const Rect& on = rects[size_t(active)];
		const int offset = std::clamp(on.x + on.w / 2 - kScreenW / 2, 0, total - kScreenW);
		for (Rect& r : rects) r.x -= offset;
	}
	return rects;
}

Rect gridCellRect(int slot) {
	const int col = slot % kGridCols;
	const int row = slot / kGridCols;
	return {kGridMarginX + col * kCellW, kContentY + row * kCellH, kCellW, kCellH};
}

Rect listRowRect(int row) {
	return {0, kContentY + row * kListRowH, kScreenW, kListRowH};
}

Rect footerFilterRect() {
	return {0, kScreenH - kFooterH, kScreenW * 2 / 5, kFooterH};
}

Rect footerSortRect() {
	return {kScreenW * 3 / 5, kScreenH - kFooterH, kScreenW * 2 / 5, kFooterH};
}

int menuFirstRow(int selected, int rows) {
	return std::clamp(selected - kMenuVisibleRows / 2, 0, std::max(0, rows - kMenuVisibleRows));
}

Rect menuRowRect(int slot) {
	return {12, kMenuTop + slot * kMenuRowH, kScreenW - 24, kMenuRowH - 3};
}

int menuRowAt(int x, int y, int selected, int rows) {
	const int first = menuFirstRow(selected, rows);
	for (int slot = 0; slot < kMenuVisibleRows && first + slot < rows; ++slot) {
		if (menuRowRect(slot).contains(x, y)) return first + slot;
	}
	return -1;
}

int tabAt(const std::vector<Rect>& tabs, int x, int y) {
	if (x < 0 || x >= kScreenW) return -1;
	for (size_t i = 0; i < tabs.size(); ++i) {
		if (tabs[i].contains(x, y)) return int(i);
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
