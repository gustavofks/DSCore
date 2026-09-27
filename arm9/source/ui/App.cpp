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
		case Action::CycleSort:
			config_.sort = nextSortKey(config_.sort);
			configChanged_ = true;
			rebuildView(currentPath);
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
	if (searching_) {
		drawKeyboardScreen(canvas, keyIndex_);
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
		case Action::CycleSort:
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
