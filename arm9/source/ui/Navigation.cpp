#include "ui/Navigation.h"

#include "ui/Layout.h"

namespace dscore {

size_t pageSize(ViewMode view) {
	return view == ViewMode::Grid ? size_t(layout::kGridPerPage) : size_t(layout::kListRows);
}

size_t pageStart(size_t cursor, ViewMode view) {
	return cursor / pageSize(view) * pageSize(view);
}

size_t moveCursor(size_t cursor, size_t count, ViewMode view, Move move) {
	if (count == 0) return 0;
	const size_t last = count - 1;
	const size_t row = (view == ViewMode::Grid) ? size_t(layout::kGridCols) : 1;
	const size_t page = pageSize(view);
	const bool grid = view == ViewMode::Grid;

	size_t step = 0;
	bool forward = false;
	switch (move) {
		case Move::Left: step = grid ? 1 : page; break;
		case Move::Right: step = grid ? 1 : page; forward = true; break;
		case Move::Up: step = row; break;
		case Move::Down: step = row; forward = true; break;
	}
	if (forward) return cursor + step > last ? last : cursor + step;
	return cursor < step ? 0 : cursor - step;
}

} // namespace dscore
