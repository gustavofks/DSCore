#pragma once

#include <string>
#include <string_view>

#include "core/Library.h"

namespace dscore {

enum class ViewMode : uint8_t { Grid, List };

// Frontend settings and where the user was, restored after returning from a game.
struct Config {
	Tab tab = Tab::all();
	Filter filter = Filter::All;
	SortKey sort = SortKey::Name;
	ViewMode view = ViewMode::Grid;
	std::string selectedPath; // game under the cursor
	std::string theme;        // theme name; empty = the default theme
	bool sound = true;        // interface sound effects
	uint32_t hiddenSystems = 0; // consoles without a tab, one bit per System value

	std::string serialize() const;
	static Config parse(std::string_view ini); // unknown or invalid values keep their defaults
};

} // namespace dscore
