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

enum class Action : uint8_t { Up, Down, Left, Right, Launch, Favorite, PrevTab, NextTab, ToggleView, CycleSort, Tap };

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

	const GameEntry* selected() const;

	// Each flag is cleared when read.
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
	std::vector<size_t> view_;
	mutable IconCache icons_; // drawing is const but warms the cache
	// The bottom screen keeps its pixels between frames: a cursor move within the page only redraws the
	// two games involved. Anything else invalidates it.
	mutable bool bottomValid_ = false;
	mutable size_t drawnCursor_ = 0;
	size_t cursor_ = 0;
	bool redraw_ = true;
	bool userDataChanged_ = false;
	bool configChanged_ = false;
};

} // namespace dscore
