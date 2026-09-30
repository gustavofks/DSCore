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
	std::string genre;         // only games of this genre (see core/Metadata.h); empty = every genre
	SortKey sort = SortKey::Name;
	ViewMode view = ViewMode::Grid;
	std::string selectedPath; // game under the cursor
	std::string theme;        // theme name; empty = the default theme
	bool sound = true;        // interface sound effects
	bool gridCovers = true;   // the grid shows box art thumbnails when there are some, else icons
	uint32_t hiddenSystems = 0; // consoles without a tab, one bit per System value

	std::string serialize() const;
	static Config parse(std::string_view ini); // unknown or invalid values keep their defaults
};

} // namespace dscore
