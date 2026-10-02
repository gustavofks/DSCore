#include "ui/App.h"

#include <algorithm>
#include <utility>

#include "core/Metadata.h"
#include "ui/Backdrop.h"
#include "ui/Keyboard.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"

namespace dscore {

App::App(const LibraryData& library, UserData& userData, Config& config)
	: library_(library), userData_(userData), config_(config) {
	refreshTabs();
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
		case Action::Launch:
			if (!currentPath.empty() && current) sound_ = Sound::Launch;
			return current ? currentPath : std::string();
		case Action::Back:
			if (detailsOpen_) { // B closes the details page first
				detailsOpen_ = false;
				sound_ = Sound::Back;
				redraw_ = true;
				break;
			}
			if (query_.empty()) break;
			query_.clear(); // B leaves the search results
			sound_ = Sound::Back;
			rebuildView(currentPath, Missing::First);
			break;
		case Action::Search:
			setSearching(true);
			break;
		case Action::Favorite:
			if (!current) break;
			sound_ = Sound::Select;
			userData_.toggleFavorite(currentPath);
			userDataChanged_ = true;
			rebuildView(currentPath);
			break;
		case Action::PrevTab:
		case Action::NextTab:
			switchTab(stepTab(tabs_, config_.tab, action == Action::NextTab ? 1 : -1), currentPath);
			break;
		case Action::NextFilter:
			setFilter(stepFilter(config_.filter, 1), currentPath);
			break;
		case Action::Details:
			detailsOpen_ = !detailsOpen_;
			sound_ = detailsOpen_ ? Sound::Select : Sound::Back;
			redraw_ = true;
			break;
		case Action::Menu:
			setMenuOpen(true);
			break;
		case Action::Tap: {
			const int tab = layout::tabAt(tabBarRects(tabs_, config_.tab), touchX, touchY);
			if (tab >= 0) {
				if (tabs_[size_t(tab)] != config_.tab) switchTab(tabs_[size_t(tab)], currentPath);
				break;
			}
			if (layout::footerFilterRect().contains(touchX, touchY)) {
				setFilter(stepFilter(config_.filter, 1), currentPath);
				break;
			}
			if (layout::footerSortRect().contains(touchX, touchY)) {
				setSort(stepSortKey(config_.sort, 1), currentPath);
				break;
			}
			const int slot = config_.view == ViewMode::Grid ? layout::gridSlotAt(touchX, touchY)
			                                                : layout::listRowAt(touchX, touchY);
			if (slot < 0) break;
			const std::string path = activate(pageStart(cursor_, config_.view) + size_t(slot));
			if (!path.empty()) sound_ = Sound::Launch;
			return path;
			break;
		}
	}
	return {};
}

void App::drawTop(Canvas& canvas) const {
	if (searching_) drawSearchScreen(canvas, state());
	else if (detailsOpen_) drawGameDetails(canvas, state());
	else drawDetailScreen(canvas, state());
}

void App::drawBottom(Canvas& canvas) const {
	const BrowserState s = state();
	const Theme& theme = *s.theme;
	if (menuOpen_) {
		if (menuPage_ == MenuPage::Main) {
			drawMenuScreen(canvas, theme, "Options", menuItems(), menuRow_, "Left/Right:Change A:Select B:Close");
		} else {
			drawMenuScreen(canvas, theme, "Consoles", menuItems(), menuRow_, "A:Show or hide the tab B:Back");
		}
		bottomValid_ = false;
		return;
	}
	if (searching_) {
		drawKeyboardScreen(canvas, theme, keyIndex_);
		bottomValid_ = false;
		return;
	}
	// With colors from the cover, moving to another game can recolor the whole screen.
	if (theme.accent != drawnAccent_) bottomValid_ = false;
	drawnAccent_ = theme.accent;
	if (bottomValid_ && pageStart(drawnCursor_, config_.view) == pageStart(cursor_, config_.view)) {
		if (drawnCursor_ != cursor_) {
			drawBrowserItem(canvas, s, drawnCursor_);
			drawBrowserItem(canvas, s, cursor_);
			drawBrowserFooter(canvas, s); // the position changed
		}
	} else {
		if (bottomValid_) bottomTransition_ = true; // replacing a page the user was looking at
		drawBrowserScreen(canvas, s);
	}
	bottomValid_ = true;
	drawnCursor_ = cursor_;
}

void App::setCover(const std::string& path, std::optional<Cover> cover) {
	coverPath_ = path;
	cover_ = std::move(cover);
	rebuildBackdrop();
	const GameEntry* game = selected();
	if (game && game->path == path) redraw_ = true;
}

void App::rebuildBackdrop() {
	backdropPath_.clear();
	if (!cover_) return;
	// The blur fades into the background of the theme that game is shown with.
	const uint16_t base = theme_->fromCover ? coverTheme(*theme_, thumbs_.accent(coverPath_)).background : theme_->background;
	backdrop_.resize(size_t(kBackdropW * kBackdropH));
	buildBackdrop(*cover_, base, backdrop_.data());
	backdropPath_ = coverPath_;
}

const Theme& App::currentTheme() const {
	if (!theme_->fromCover) return *theme_;
	const GameEntry* game = selected();
	current_ = coverTheme(*theme_, game ? thumbs_.accent(game->path) : 0);
	return current_;
}

void App::setThumbSource(std::vector<ThumbEntry> entries, ThumbCache::Loader loader) {
	thumbs_.setSource(std::move(entries), std::move(loader));
	rebuildBackdrop(); // the accent colors come with the thumbnails
	invalidate();
}

void App::setHasSave(const std::string& path, bool hasSave) {
	savePath_ = path;
	hasSave_ = hasSave;
	const GameEntry* game = selected();
	if (game && game->path == path) redraw_ = true;
}

void App::setTheme(const Theme& theme) {
	theme_ = &theme;
	rebuildBackdrop();
	invalidate();
}

void App::setThemes(const std::vector<Theme>& themes) {
	themes_ = &themes;
	setTheme(findTheme(themes, config_.theme));
}

void App::libraryChanged() {
	refreshTabs();
	icons_.clear();
	cover_.reset();
	coverPath_.clear();
	backdropPath_.clear();
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

Sound App::takeSound() { return std::exchange(sound_, Sound::None); }
bool App::takeBottomTransition() { return std::exchange(bottomTransition_, false); }
bool App::takeRedraw() { return std::exchange(redraw_, false); }
bool App::takeUserDataChanged() { return std::exchange(userDataChanged_, false); }
bool App::takeConfigChanged() { return std::exchange(configChanged_, false); }

BrowserState App::state() const {
	BrowserState s;
	s.theme = &currentTheme();
	s.library = &library_;
	s.userData = &userData_;
	s.icons = &icons_;
	s.thumbs = config_.gridCovers && !thumbs_.empty() ? &thumbs_ : nullptr;
	const GameEntry* game = selected();
	s.cover = (cover_ && game && game->path == coverPath_) ? &*cover_ : nullptr;
	s.backdrop = game && game->path == backdropPath_ ? backdrop_.data() : nullptr;
	s.hasSave = hasSave_ && game && game->path == savePath_;
	s.view = &view_;
	s.cursor = cursor_;
	s.tabs = &tabs_;
	s.tab = config_.tab;
	s.filter = config_.filter;
	s.genre = config_.genre;
	s.sort = config_.sort;
	s.mode = config_.view;
	s.query = query_;
	return s;
}

// Recomputes the visible games and keeps keepPath under the cursor when it is still listed; otherwise
// the cursor goes to the first game or stays at the same position, clamped.
void App::rebuildView(const std::string& keepPath, Missing missing) {
	view_ = libraryView(library_.games, userData_, config_.tab, config_.filter, config_.sort, query_, config_.genre);
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

// Recomputes the tab bar and the genre list; a saved tab whose console has no games left falls back to
// All, and a saved genre that no game has any more is cleared.
void App::refreshTabs() {
	genres_ = genresOf(library_.games);
	if (!config_.genre.empty() && std::find(genres_.begin(), genres_.end(), config_.genre) == genres_.end()) {
		config_.genre.clear();
		configChanged_ = true;
	}
	tabs_ = availableTabs(library_.games, config_.hiddenSystems);
	if (std::find(tabs_.begin(), tabs_.end(), config_.tab) != tabs_.end()) return;
	config_.tab = Tab::all();
	configChanged_ = true;
}

void App::switchTab(Tab tab, const std::string& currentPath) {
	config_.tab = tab;
	sound_ = Sound::Select;
	configChanged_ = true;
	rebuildView(currentPath, Missing::First);
}

void App::setFilter(Filter filter, const std::string& currentPath) {
	config_.filter = filter;
	sound_ = Sound::Select;
	configChanged_ = true;
	rebuildView(currentPath, Missing::First);
}

void App::setSort(SortKey sort, const std::string& currentPath) {
	config_.sort = sort;
	sound_ = Sound::Select;
	configChanged_ = true;
	rebuildView(currentPath, Missing::First);
}

void App::select(size_t cursor) {
	if (cursor == cursor_ || cursor >= view_.size()) return;
	cursor_ = cursor;
	sound_ = Sound::Move;
	config_.selectedPath = library_.games[view_[cursor_]].path;
	configChanged_ = true;
	redraw_ = true;
}

std::vector<MenuItem> App::menuItems() const {
	if (menuPage_ == MenuPage::Consoles) {
		std::vector<MenuItem> items;
		for (System system : menuConsoles()) {
			const bool hidden = config_.hiddenSystems & (1u << int(system));
			items.push_back({systemInfo(system).name, hidden ? "Hidden" : "Shown"});
		}
		items.push_back({"Back", ""});
		return items;
	}
	std::vector<MenuItem> items(kMenuRows);
	items[kFilterRow] = {"Show", filterLabel(config_.filter)};
	items[kGenreRow] = {"Genre", genres_.empty() ? "-" : config_.genre.empty() ? "All" : config_.genre};
	items[kSortRow] = {"Sort by", sortKeyLabel(config_.sort)};
	items[kViewRow] = {"View", config_.view == ViewMode::Grid ? "Grid" : "List"};
	items[kGridArtRow] = {"Grid art", config_.gridCovers ? "Box art" : "Icons"};
	items[kThemeRow] = {"Theme", theme_->name};
	items[kSoundRow] = {"Sounds", config_.sound ? "On" : "Off"};
	items[kConsolesRow] = {"Consoles...", ""};
	items[kRandomRow] = {"Random game", ""};
	items[kRebuildRow] = {"Rebuild library", ""};
	items[kCloseRow] = {"Close", ""};
	return items;
}

// Options menu: up/down pick a row, left/right change its value, A changes values or runs the action,
// B and START close it.
void App::handleMenu(Action action, int touchX, int touchY) {
	switch (action) {
		case Action::Up:
			menuRow_ = std::max(0, menuRow_ - 1);
			sound_ = Sound::Move;
			break;
		case Action::Down:
			menuRow_ = std::min(menuRowCount() - 1, menuRow_ + 1);
			sound_ = Sound::Move;
			break;
		case Action::Left:
		case Action::Right:
		case Action::Launch: {
			const int direction = action == Action::Left ? -1 : 1;
			if (menuPage_ == MenuPage::Main) activateMenuRow(menuRow_, direction);
			else activateConsoleRow(menuRow_);
			break;
		}
		case Action::Back:
		case Action::Menu:
			if (menuPage_ == MenuPage::Consoles) {
				menuPage_ = MenuPage::Main;
				menuRow_ = kConsolesRow;
				sound_ = Sound::Back;
			} else {
				setMenuOpen(false);
			}
			break;
		case Action::Tap: {
			const int row = layout::menuRowAt(touchX, touchY, menuRow_, menuRowCount());
			if (row < 0) return;
			menuRow_ = row;
			if (menuPage_ == MenuPage::Main) activateMenuRow(row, 1);
			else activateConsoleRow(row);
			break;
		}
		default: return;
	}
	redraw_ = true;
}

void App::activateMenuRow(int row, int direction) {
	sound_ = Sound::Select;
	const GameEntry* current = selected();
	const std::string currentPath = current ? current->path : config_.selectedPath;
	switch (row) {
		case kFilterRow:
			config_.filter = stepFilter(config_.filter, direction);
			configChanged_ = true;
			rebuildView(currentPath);
			break;
		case kGenreRow: {
			if (genres_.empty()) break;
			// Cycles through "All" (index 0) and each genre.
			const int count = int(genres_.size()) + 1;
			int index = 0;
			for (int i = 0; i < int(genres_.size()); ++i) {
				if (genres_[size_t(i)] == config_.genre) index = i + 1;
			}
			index = ((index + direction) % count + count) % count;
			config_.genre = index == 0 ? std::string() : genres_[size_t(index - 1)];
			configChanged_ = true;
			rebuildView(currentPath);
			break;
		}
		case kSortRow:
			config_.sort = stepSortKey(config_.sort, direction);
			configChanged_ = true;
			rebuildView(currentPath);
			break;
		case kViewRow:
			config_.view = config_.view == ViewMode::Grid ? ViewMode::List : ViewMode::Grid;
			configChanged_ = true;
			break;
		case kGridArtRow:
			config_.gridCovers = !config_.gridCovers;
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
		case kSoundRow:
			config_.sound = !config_.sound;
			configChanged_ = true;
			break;
		case kConsolesRow:
			if (direction < 0) break;
			menuPage_ = MenuPage::Consoles;
			menuRow_ = 0;
			break;
		case kRandomRow:
			if (direction < 0) break;
			pickRandomGame();
			break;
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

int App::menuRowCount() const {
	return menuPage_ == MenuPage::Main ? int(kMenuRows) : int(menuConsoles().size()) + 1; // + Back
}

std::vector<System> App::menuConsoles() const {
	// Every console with games, hidden or not, in tab order.
	std::vector<System> systems;
	for (const Tab& tab : availableTabs(library_.games)) {
		if (tab.kind == Tab::Kind::Console) systems.push_back(tab.system);
	}
	return systems;
}

// Consoles page: A shows or hides the console's tab; the last row goes back.
void App::activateConsoleRow(int row) {
	const std::vector<System> systems = menuConsoles();
	if (row >= int(systems.size())) {
		menuPage_ = MenuPage::Main;
		menuRow_ = kConsolesRow;
		sound_ = Sound::Back;
		return;
	}
	sound_ = Sound::Select;
	config_.hiddenSystems ^= 1u << int(systems[size_t(row)]);
	configChanged_ = true;
	const GameEntry* current = selected();
	const std::string currentPath = current ? current->path : config_.selectedPath;
	refreshTabs();
	rebuildView(currentPath);
}

// Selects a random game of the current list (another one when there is a choice) and closes the menu.
void App::pickRandomGame() {
	if (view_.empty()) return;
	random_ ^= random_ << 13;
	random_ ^= random_ >> 17;
	random_ ^= random_ << 5;
	size_t pick = random_ % view_.size();
	if (pick == cursor_ && view_.size() > 1) pick = (pick + 1 + random_ / 7 % (view_.size() - 1)) % view_.size();
	setMenuOpen(false);
	select(pick);
	sound_ = Sound::Launch;
}

void App::setMenuOpen(bool open) {
	menuOpen_ = open;
	if (open) menuPage_ = MenuPage::Main;
	sound_ = open ? Sound::Select : Sound::Back;
	bottomValid_ = false;
	redraw_ = true;
}

// While searching: the D-pad moves on the keyboard, A or a touch presses a key, B deletes (or leaves
// when there is nothing to delete) and START finishes, like the OK key.
void App::handleSearch(Action action, int touchX, int touchY) {
	switch (action) {
		case Action::Up: keyIndex_ = moveKey(keyIndex_, Move::Up); sound_ = Sound::Move; break;
		case Action::Down: keyIndex_ = moveKey(keyIndex_, Move::Down); sound_ = Sound::Move; break;
		case Action::Left: keyIndex_ = moveKey(keyIndex_, Move::Left); sound_ = Sound::Move; break;
		case Action::Right: keyIndex_ = moveKey(keyIndex_, Move::Right); sound_ = Sound::Move; break;
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
	sound_ = Sound::Select;
	switch (applyKey(keyboardKeys()[size_t(index)], query_)) {
		case KeyResult::Edited: rebuildView(currentPath, Missing::First); break;
		case KeyResult::Done: setSearching(false); break;
		case KeyResult::None: break;
	}
}

void App::setSearching(bool searching) {
	searching_ = searching;
	sound_ = searching ? Sound::Select : Sound::Back;
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
