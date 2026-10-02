#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "core/Config.h"
#include "core/Cover.h"
#include "core/LibraryCache.h"
#include "core/UserData.h"
#include "gfx/Canvas.h"
#include "ui/IconCache.h"
#include "ui/ThumbCache.h"
#include "ui/Theme.h"

namespace dscore {

// What the screens show; the views only read it.
struct BrowserState {
	const Theme* theme = nullptr;
	const LibraryData* library = nullptr;
	const UserData* userData = nullptr;
	IconCache* icons = nullptr;                 // decoded icons, filled while drawing
	ThumbCache* thumbs = nullptr;               // grid box art, filled while drawing; null to show icons
	const Cover* cover = nullptr;               // box art of the selected game, when loaded
	const uint16_t* backdrop = nullptr;         // blurred cover for the details screen (256x192), when built
	bool hasSave = false;                       // the selected game has a save file
	std::string_view query;                     // active search filter, empty when none
	const std::vector<size_t>* view = nullptr; // indexes into library->games for the current tab
	size_t cursor = 0;                          // position in view
	const std::vector<Tab>* tabs = nullptr;    // the tab bar, left to right
	Tab tab = Tab::all();
	Filter filter = Filter::All;
	std::string_view genre;                     // genre filter, empty when none
	SortKey sort = SortKey::Name;
	ViewMode mode = ViewMode::Grid;
};

// Where each tab chip lies while active is selected (see layout::chipRects).
std::vector<Rect> tabBarRects(const std::vector<Tab>& tabs, Tab active);

// The game's banner icon, or a tile in the system color with the title's initials, drawn as a square of
// size pixels: 16 (half size), 32 or any multiple of 32.
void drawGameTile(Canvas& canvas, const Theme& theme, const LibraryData& library, IconCache& icons, const GameEntry& game,
	int x, int y, int size);

// Top screen: details of the selected game.
void drawDetailScreen(Canvas& canvas, const BrowserState& state);

// Bottom screen: tab bar, grid or list, footer.
void drawBrowserScreen(Canvas& canvas, const BrowserState& state);

// Redraws one game of the current page (a grid cell or list row) over what drawBrowserScreen drew,
// e.g. to move the selection without redrawing the whole screen.
void drawBrowserItem(Canvas& canvas, const BrowserState& state, size_t index);

// Top screen while searching: the query and the first matching games.
void drawSearchScreen(Canvas& canvas, const BrowserState& state);

// Bottom screen while searching: the on-screen keyboard with the key under the cursor highlighted.
void drawKeyboardScreen(Canvas& canvas, const Theme& theme, int selectedKey);

struct MenuItem {
	std::string label;
	std::string value; // empty for actions
};

// Bottom screen while the options menu is open: title, rows scrolled to keep selected in view.
void drawMenuScreen(Canvas& canvas, const Theme& theme, const std::string& title, const std::vector<MenuItem>& items,
	int selected, const std::string& hint);

// Full-screen message, e.g. while indexing or after an error.
void drawMessageScreen(Canvas& canvas, const Theme& theme, const std::string& title, const std::vector<std::string>& lines);

} // namespace dscore
