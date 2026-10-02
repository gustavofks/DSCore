#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/Systems.h"
#include "gfx/Canvas.h"

namespace dscore {

// Colors the views draw with. Built-in themes ship with DSCore; more can be added as INI files in
// sd:/_nds/DSCore/themes without rebuilding.
struct Theme {
	std::string name;
	uint16_t background;
	uint16_t surface;
	uint16_t surfaceHigh;
	uint16_t text;
	uint16_t muted;
	uint16_t accent;
	uint16_t favorite;
	uint16_t ndsShades[4]; // generated tiles pick one shade per game so neighbours differ
	uint16_t gbaShades[4];
	bool fromCover = false; // colors follow the selected game's cover (see coverTheme)
};

// The theme for a game whose cover has this accent color (a ThumbEntry accent): dark tints of the accent
// for the background and surfaces, light text, the accent itself for selection. base supplies the rest,
// and is returned unchanged when accent is 0 (unknown).
Theme coverTheme(const Theme& base, uint16_t accent);

// Four shades for generated tiles of system's games: the theme's own for DS and GBA, fixed mid-tones
// (readable on any background) for the other consoles.
const uint16_t* tileShades(const Theme& theme, System system);

// The first one is the default.
const std::vector<Theme>& builtInThemes();

// The theme called name, or the first one when there is none.
const Theme& findTheme(const std::vector<Theme>& themes, const std::string& name);

// "#RRGGBB" (surrounding spaces allowed) to a DS color.
bool parseColor(std::string_view text, uint16_t& color);

// A theme file: [theme] name, and [colors] background / surface / surface_high / text / muted /
// accent / favorite as #RRGGBB. Anything missing or invalid keeps base's value; the name falls back to
// fallbackName, then to base's name.
Theme parseTheme(std::string_view ini, const Theme& base, const std::string& fallbackName = {});

} // namespace dscore
