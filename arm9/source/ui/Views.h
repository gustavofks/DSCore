#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/Config.h"
#include "core/Cover.h"
#include "core/LibraryCache.h"
#include "core/UserData.h"
#include "gfx/Canvas.h"
#include "ui/IconCache.h"

namespace dscore {

// What the screens show; the views only read it.
struct BrowserState {
	const LibraryData* library = nullptr;
	const UserData* userData = nullptr;
	IconCache* icons = nullptr;                 // decoded icons, filled while drawing
	const Cover* cover = nullptr;               // box art of the selected game, when loaded
	const std::vector<size_t>* view = nullptr; // indexes into library->games for the current tab
	size_t cursor = 0;                          // position in view
	Tab tab = Tab::All;
	SortKey sort = SortKey::Name;
	ViewMode mode = ViewMode::Grid;
};

// The game's banner icon, or a tile in the system color with the title's initials, drawn as a square of
// size pixels: 16 (half size), 32 or any multiple of 32.
void drawGameTile(Canvas& canvas, const LibraryData& library, IconCache& icons, const GameEntry& game, int x, int y,
	int size);

// Top screen: details of the selected game.
void drawDetailScreen(Canvas& canvas, const BrowserState& state);

// Bottom screen: tab bar, grid or list, footer.
void drawBrowserScreen(Canvas& canvas, const BrowserState& state);

// Redraws one game of the current page (a grid cell or list row) over what drawBrowserScreen drew,
// e.g. to move the selection without redrawing the whole screen.
void drawBrowserItem(Canvas& canvas, const BrowserState& state, size_t index);

// Full-screen message, e.g. while indexing or after an error.
void drawMessageScreen(Canvas& canvas, const std::string& title, const std::vector<std::string>& lines);

} // namespace dscore
