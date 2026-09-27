#include "ui/App.h"

#include <algorithm>
#include <utility>

#include "ui/Keyboard.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"

namespace dscore {

App::App(const LibraryData& library, UserData& userData, Config& config)
	: library_(library), userData_(userData), config_(config) {
	rebuildView(config_.selectedPath, Missing::First);
}

std::string App::handle(Action action, int touchX, int touchY) {
	if (menuOpen_) {
		handleMenu(action, touchX, touchY);
		return {};
	}
	if (searching_) {
		handleSearch(action, touchX, touchY);
		return {};
	}
	const GameEntry* current = selected();
	// An empty tab has no selection; keep following the last selected game.
	const std::string currentPath = current ? current->path : config_.selectedPath;

	switch (action) {
		case Action::Up: select(moveCursor(cursor_, view_.size(), config_.view, Move::Up)); break;
		case Action::Down: select(moveCursor(cursor_, view_.size(), config_.view, Move::Down)); break;
		case Action::Left: select(moveCursor(cursor_, view_.size(), config_.view, Move::Left)); break;
		case Action::Right: select(moveCursor(cursor_, view_.size(), config_.view, Move::Right)); break;
		case Action::Launch: return currentPath;
		case Action::Back:
			if (query_.empty()) break;
			query_.clear(); // B leaves the search results
			rebuildView(currentPath, Missing::First);
			break;
		case Action::Search:
			setSearching(true);
			break;
		case Action::Favorite:
			if (!current) break;
			userData_.toggleFavorite(currentPath);
			userDataChanged_ = true;
			rebuildView(currentPath);
			break;
		case Action::PrevTab:
		case Action::NextTab:
			config_.tab = action == Action::NextTab ? nextTab(config_.tab) : previousTab(config_.tab);
			configChanged_ = true;
			rebuildView(currentPath, Missing::First);
			break;
		case Action::ToggleView:
			config_.view = config_.view == ViewMode::Grid ? ViewMode::List : ViewMode::Grid;
			configChanged_ = true;
			bottomValid_ = false;
			redraw_ = true;
			break;
		case Action::Menu:
			setMenuOpen(true);
			break;
		case Action::Tap: {
			const int tab = layout::tabAt(touchX, touchY);
			if (tab >= 0) {
				if (Tab(tab) == config_.tab) break;
				config_.tab = Tab(tab);
				configChanged_ = true;
				rebuildView(currentPath, Missing::First);
				break;
			}
			const int slot = config_.view == ViewMode::Grid ? layout::gridSlotAt(touchX, touchY)
			                                                : layout::listRowAt(touchX, touchY);
			if (slot >= 0) return activate(pageStart(cursor_, config_.view) + size_t(slot));
			break;
		}
	}
	return {};
}

void App::drawTop(Canvas& canvas) const {
	if (searching_) drawSearchScreen(canvas, state());
	else drawDetailScreen(canvas, state());
}

void App::drawBottom(Canvas& canvas) const {
	const BrowserState s = state();
	if (menuOpen_) {
		drawMenuScreen(canvas, *theme_, menuItems(), menuRow_);
		bottomValid_ = false;
		return;
	}
	if (searching_) {
		drawKeyboardScreen(canvas, *theme_, keyIndex_);
		bottomValid_ = false;
		return;
	}
	if (bottomValid_ && pageStart(drawnCursor_, config_.view) == pageStart(cursor_, config_.view)) {
		if (drawnCursor_ != cursor_) {
			drawBrowserItem(canvas, s, drawnCursor_);
			drawBrowserItem(canvas, s, cursor_);
		}
	} else {
		drawBrowserScreen(canvas, s);
	}
	bottomValid_ = true;
	drawnCursor_ = cursor_;
}

void App::setCover(const std::string& path, std::optional<Cover> cover) {
	coverPath_ = path;
	cover_ = std::move(cover);
	const GameEntry* game = selected();
	if (game && game->path == path) redraw_ = true;
}

void App::setTheme(const Theme& theme) {
	theme_ = &theme;
	invalidate();
}

void App::setThemes(const std::vector<Theme>& themes) {
	themes_ = &themes;
	setTheme(findTheme(themes, config_.theme));
}

void App::libraryChanged() {
	icons_.clear();
	cover_.reset();
	coverPath_.clear();
	rebuildView(config_.selectedPath, Missing::First);
	invalidate();
}

bool App::takeRebuildRequest() { return std::exchange(rebuildRequested_, false); }

void App::invalidate() {
	bottomValid_ = false;
	redraw_ = true;
}

const GameEntry* App::selected() const {
	return cursor_ < view_.size() ? &library_.games[view_[cursor_]] : nullptr;
}

bool App::takeRedraw() { return std::exchange(redraw_, false); }
bool App::takeUserDataChanged() { return std::exchange(userDataChanged_, false); }
bool App::takeConfigChanged() { return std::exchange(configChanged_, false); }

BrowserState App::state() const {
	BrowserState s;
	s.theme = theme_;
	s.library = &library_;
	s.userData = &userData_;
	s.icons = &icons_;
	const GameEntry* game = selected();
	s.cover = (cover_ && game && game->path == coverPath_) ? &*cover_ : nullptr;
	s.view = &view_;
	s.cursor = cursor_;
	s.tab = config_.tab;
	s.sort = config_.sort;
	s.mode = config_.view;
	s.query = query_;
	return s;
}

// Recomputes the visible games and keeps keepPath under the cursor when it is still listed; otherwise
// the cursor goes to the first game or stays at the same position, clamped.
void App::rebuildView(const std::string& keepPath, Missing missing) {
	view_ = libraryView(library_.games, userData_, config_.tab, config_.sort, query_);
	size_t cursor = missing == Missing::First ? 0 : std::min(cursor_, view_.empty() ? 0 : view_.size() - 1);
	for (size_t i = 0; i < view_.size(); ++i) {
		if (library_.games[view_[i]].path == keepPath) {
			cursor = i;
			break;
		}
	}
	cursor_ = view_.empty() ? 0 : cursor;
	bottomValid_ = false;
	if (const GameEntry* game = selected(); game && game->path != config_.selectedPath) {
		config_.selectedPath = game->path;
		configChanged_ = true;
	}
	redraw_ = true;
}

void App::select(size_t cursor) {
	if (cursor == cursor_ || cursor >= view_.size()) return;
	cursor_ = cursor;
	config_.selectedPath = library_.games[view_[cursor_]].path;
	configChanged_ = true;
	redraw_ = true;
}

std::vector<MenuItem> App::menuItems() const {
	std::vector<MenuItem> items(kMenuRows);
	items[kSortRow] = {"Sort by", sortKeyLabel(config_.sort)};
	items[kViewRow] = {"View", config_.view == ViewMode::Grid ? "Grid" : "List"};
	items[kThemeRow] = {"Theme", theme_->name};
	items[kRebuildRow] = {"Rebuild library", ""};
	items[kCloseRow] = {"Close", ""};
	return items;
}

// Options menu: up/down pick a row, left/right change its value, A changes values or runs the action,
// B and START close it.
void App::handleMenu(Action action, int touchX, int touchY) {
	switch (action) {
		case Action::Up: menuRow_ = std::max(0, menuRow_ - 1); break;
		case Action::Down: menuRow_ = std::min(int(kMenuRows) - 1, menuRow_ + 1); break;
		case Action::Left: activateMenuRow(menuRow_, -1); break;
		case Action::Right:
		case Action::Launch: activateMenuRow(menuRow_, 1); break;
		case Action::Back:
		case Action::Menu: setMenuOpen(false); break;
		case Action::Tap: {
			const int row = layout::menuRowAt(touchX, touchY, kMenuRows);
			if (row < 0) return;
			menuRow_ = row;
			activateMenuRow(row, 1);
			break;
		}
		default: return;
	}
	redraw_ = true;
}

void App::activateMenuRow(int row, int direction) {
	const GameEntry* current = selected();
	const std::string currentPath = current ? current->path : config_.selectedPath;
	switch (row) {
		case kSortRow: {
			constexpr int kSortKeys = 3;
			config_.sort = SortKey((int(config_.sort) + kSortKeys + direction) % kSortKeys);
			configChanged_ = true;
			rebuildView(currentPath);
			break;
		}
		case kViewRow:
			config_.view = config_.view == ViewMode::Grid ? ViewMode::List : ViewMode::Grid;
			configChanged_ = true;
			break;
		case kThemeRow: {
			if (!themes_ || themes_->empty()) break;
			const int count = int(themes_->size());
			int index = 0;
			while (index < count && &(*themes_)[size_t(index)] != theme_) ++index;
			index = ((index % count) + count + direction) % count;
			setTheme((*themes_)[size_t(index)]);
			config_.theme = theme_->name;
			configChanged_ = true;
			break;
		}
		case kRebuildRow:
			if (direction < 0) break;
			rebuildRequested_ = true;
			setMenuOpen(false);
			break;
		case kCloseRow:
			if (direction > 0) setMenuOpen(false);
			break;
	}
}

void App::setMenuOpen(bool open) {
	menuOpen_ = open;
	bottomValid_ = false;
	redraw_ = true;
}

// While searching: the D-pad moves on the keyboard, A or a touch presses a key, B deletes (or leaves
// when there is nothing to delete) and START finishes, like the OK key.
void App::handleSearch(Action action, int touchX, int touchY) {
	switch (action) {
		case Action::Up: keyIndex_ = moveKey(keyIndex_, Move::Up); break;
		case Action::Down: keyIndex_ = moveKey(keyIndex_, Move::Down); break;
		case Action::Left: keyIndex_ = moveKey(keyIndex_, Move::Left); break;
		case Action::Right: keyIndex_ = moveKey(keyIndex_, Move::Right); break;
		case Action::Launch: pressKey(keyIndex_); break;
		case Action::Back:
			if (query_.empty()) {
				setSearching(false);
			} else {
				pressKey(keyIndexFor('\b'));
			}
			break;
		case Action::Menu:
		case Action::Search: setSearching(false); break;
		case Action::Tap: {
			const int key = keyAt(touchX, touchY);
			if (key >= 0) {
				keyIndex_ = key;
				pressKey(key);
			}
			break;
		}
		default: return;
	}
	redraw_ = true;
}

void App::pressKey(int index) {
	const GameEntry* current = selected();
	const std::string currentPath = current ? current->path : config_.selectedPath;
	switch (applyKey(keyboardKeys()[size_t(index)], query_)) {
		case KeyResult::Edited: rebuildView(currentPath, Missing::First); break;
		case KeyResult::Done: setSearching(false); break;
		case KeyResult::None: break;
	}
}

void App::setSearching(bool searching) {
	searching_ = searching;
	if (searching && keyIndex_ == 0) keyIndex_ = keyIndexFor('A');
	bottomValid_ = false;
	redraw_ = true;
}

// Touching a game selects it; touching the selected game launches it.
std::string App::activate(size_t index) {
	if (index >= view_.size()) return {};
	if (index == cursor_) return library_.games[view_[index]].path;
	select(index);
	return {};
}

} // namespace dscore
