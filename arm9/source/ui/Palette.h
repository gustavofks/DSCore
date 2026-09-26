#pragma once

#include "gfx/Canvas.h"

namespace dscore::palette {

// Built-in dark theme; themes loaded from the SD card replace these in a later phase.
constexpr uint16_t kBackground = rgb(2, 3, 5);
constexpr uint16_t kSurface = rgb(5, 6, 9);
constexpr uint16_t kSurfaceHigh = rgb(8, 10, 14);
constexpr uint16_t kText = rgb(29, 29, 30);
constexpr uint16_t kMuted = rgb(16, 17, 20);
constexpr uint16_t kAccent = rgb(9, 19, 31);
constexpr uint16_t kFavorite = rgb(31, 25, 6);
constexpr uint16_t kNds = rgb(21, 7, 8);
constexpr uint16_t kGba = rgb(13, 9, 23);

} // namespace dscore::palette
