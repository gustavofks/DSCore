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
	size_t cursor_ = 0;
	bool redraw_ = true;
	bool userDataChanged_ = false;
	bool configChanged_ = false;
};

} // namespace dscore
