#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/Config.h"
#include "core/LibraryCache.h"
#include "core/UserData.h"
#include "gfx/Canvas.h"
#include "ui/Views.h"

namespace dscore {

// Interface sound the caller should play after an action.
enum class Sound : uint8_t { None, Move, Select, Back, Launch };

enum class Action : uint8_t {
	Up, Down, Left, Right, Launch, Back, Favorite, PrevTab, NextTab, ToggleView, Menu, Search, Tap
};

// Library browser state and input handling, independent of the hardware: the caller feeds actions,
// draws when asked and persists user data and config when they change.
class App {
public:
	App(const LibraryData& library, UserData& userData, Config& config);

	// Applies one input (touch coordinates are only used by Tap). Returns the ROM path to launch, or an
	// empty string.
	std::string handle(Action action, int touchX = 0, int touchY = 0);

	void drawTop(Canvas& canvas) const;
	void drawBottom(Canvas& canvas) const;

	// Call after something else drew on the screens: the next frame redraws everything.
	void invalidate();

	void setTheme(const Theme& theme);
	const Theme& theme() const { return *theme_; }

	// Themes the options menu cycles through; selects the one named in the config.
	void setThemes(const std::vector<Theme>& themes);

	// The library was replaced (rebuilt): recompute the view and forget cached icons and cover.
	void libraryChanged();

	bool menuOpen() const { return menuOpen_; }

	// True once after the user picked "Rebuild library" in the options menu.
	bool takeRebuildRequest();

	const GameEntry* selected() const;
	bool searching() const { return searching_; }
	const std::string& query() const { return query_; }

	// Box art for the game at path (nullopt when it has none); shown while that game is selected.
	void setCover(const std::string& path, std::optional<Cover> cover);

	// Each flag is cleared when read.
	Sound takeSound();
	bool takeBottomTransition(); // the bottom screen switched to another page or tab
	bool takeRedraw();
	bool takeUserDataChanged();
	bool takeConfigChanged();

private:
	BrowserState state() const;
	enum class Missing { First, Clamp }; // where the cursor goes when the kept game is not listed
	void rebuildView(const std::string& keepPath, Missing missing = Missing::Clamp);
	void select(size_t cursor);
	std::string activate(size_t index);

	const LibraryData& library_;
	UserData& userData_;
	Config& config_;
	const Theme* theme_ = &builtInThemes()[0];
	std::vector<size_t> view_;
	mutable IconCache icons_; // drawing is const but warms the cache
	enum MenuRow { kSortRow, kViewRow, kThemeRow, kSoundRow, kRebuildRow, kCloseRow, kMenuRows };
	std::vector<MenuItem> menuItems() const;
	void handleMenu(Action action, int touchX, int touchY);
	void activateMenuRow(int row, int direction);
	void setMenuOpen(bool open);
	void handleSearch(Action action, int touchX, int touchY);
	void pressKey(int index);
	void setSearching(bool searching);

	bool menuOpen_ = false;
	int menuRow_ = 0;
	bool rebuildRequested_ = false;
	const std::vector<Theme>* themes_ = nullptr;
	bool searching_ = false; // the keyboard is on screen
	std::string query_;      // filter applied to every tab, typed with the keyboard
	int keyIndex_ = 0;
	std::string coverPath_;
	std::optional<Cover> cover_;
	// The bottom screen keeps its pixels between frames: a cursor move within the page only redraws the
	// two games involved. Anything else invalidates it.
	mutable bool bottomValid_ = false;
	mutable size_t drawnCursor_ = 0;
	size_t cursor_ = 0;
	bool redraw_ = true;
	Sound sound_ = Sound::None;
	mutable bool bottomTransition_ = false;
	bool userDataChanged_ = false;
	bool configChanged_ = false;
};

} // namespace dscore
