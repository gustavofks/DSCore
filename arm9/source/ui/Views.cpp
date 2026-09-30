#include "ui/Views.h"

#include <algorithm>

#include "core/RomMedia.h"
#include "core/Version.h"
#include "core/Text.h"
#include "core/Titles.h"
#include "ui/Keyboard.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"

namespace dscore {

namespace {

using namespace layout;

constexpr int kHeaderH = 16;
constexpr int kHintBarY = kScreenH - kFooterH;
constexpr uint32_t kFirstRtcTime = 1000000000; // below this, lastPlayed is an imported rank, not a date
constexpr uint16_t kTileText = rgb(31, 31, 31); // generated tiles use mid-tone colors in every theme
// Portuguese badge in the colors of the Brazilian flag, the same in every theme.
constexpr uint16_t kBadgeGreen = rgb(0, 15, 6);
constexpr uint16_t kBadgeYellow = rgb(31, 28, 2);

// 9x9 star, bit 8 = leftmost pixel.
constexpr uint16_t kStar[9] = {0x010, 0x010, 0x038, 0x1FF, 0x0FE, 0x07C, 0x06C, 0x0C6, 0x082};

// 5-pixel-wide triangle pointing up (direction -1), down (1), left (-2) or right (2).
void drawArrow(Canvas& canvas, int x, int y, int direction, uint16_t color) {
	for (int i = 0; i < 3; ++i) {
		const int len = 5 - 2 * i;
		switch (direction) {
			case -1: canvas.fillRect({x + i, y + 2 - i, len, 1}, color); break;
			case 1: canvas.fillRect({x + i, y + i, len, 1}, color); break;
			case -2: canvas.fillRect({x + 2 - i, y + i, 1, len}, color); break;
			default: canvas.fillRect({x + i, y + i, 1, len}, color); break;
		}
	}
}

void drawStar(Canvas& canvas, int x, int y, uint16_t color) {
	for (int row = 0; row < 9; ++row) {
		for (int col = 0; col < 9; ++col) {
			if (kStar[row] & (0x100 >> col)) canvas.fillRect({x + col, y + row, 1, 1}, color);
		}
	}
}

// Small-font label on a filled box at (x, y); returns the x just past it.
int drawBadge(Canvas& canvas, int x, int y, const std::string& text, uint16_t background, uint16_t color) {
	const int w = textWidth(smallFont(), text) + 6;
	canvas.fillRect({x, y, w, smallFont().height + 1}, background);
	canvas.drawText(smallFont(), x + 3, y + 1, text, color);
	return x + w;
}

void drawCentered(Canvas& canvas, const Font& font, const Rect& r, const std::string& text, uint16_t color) {
	const std::string fitted = ellipsize(font, text, r.w);
	canvas.drawText(font, r.x + (r.w - textWidth(font, fitted)) / 2, r.y + (r.h - font.height) / 2, fitted, color);
}

const GameEntry* selectedGame(const BrowserState& state) {
	if (!state.view || state.cursor >= state.view->size()) return nullptr;
	return &state.library->games[(*state.view)[state.cursor]];
}

bool isFavorite(const BrowserState& state, const GameEntry& game) {
	const GameStats* stats = state.userData->find(game.path);
	return stats && stats->favorite;
}

std::string sizeText(uint32_t bytes) {
	if (bytes >= (1u << 20)) return std::to_string(bytes >> 20) + " MB";
	return std::to_string(bytes >> 10) + " KB";
}

uint16_t tileColor(const Theme& theme, const GameEntry& game) {
	uint32_t hash = 2166136261u;
	for (char c : game.title) hash = (hash ^ uint8_t(c)) * 16777619u;
	return tileShades(theme, game.system)[hash % 4];
}

std::string fileName(const std::string& path) {
	const size_t slash = path.find_last_of('/');
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string playedText(const GameStats* stats) {
	if (!stats || stats->timesPlayed == 0) return "Not played yet";
	if (stats->timesPlayed == 1) return "Played once";
	return "Played " + std::to_string(stats->timesPlayed) + " times";
}

void drawTabs(Canvas& canvas, const Theme& theme, const std::vector<Tab>& tabs, Tab active) {
	const std::vector<Rect> rects = tabBarRects(tabs, active);
	for (size_t i = 0; i < tabs.size(); ++i) {
		const Rect& r = rects[i];
		if (r.x >= kScreenW || r.x + r.w <= 0) continue;
		const bool on = tabs[i] == active;
		canvas.fillRect({r.x + 1, r.y, r.w - 2, r.h}, on ? theme.accent : theme.surface);
		drawCentered(canvas, smallFont(), r, tabLabel(tabs[i]), on ? theme.text : theme.muted);
	}
	// Arrows over the edges tell that more tabs are one L/R away.
	if (!rects.empty() && rects.front().x < 0) {
		canvas.fillRect({0, 0, 9, kTabBarH}, theme.background);
		drawArrow(canvas, 2, kTabBarH / 2 - 2, -2, theme.accent);
	}
	if (!rects.empty() && rects.back().x + rects.back().w > kScreenW) {
		canvas.fillRect({kScreenW - 9, 0, 9, kTabBarH}, theme.background);
		drawArrow(canvas, kScreenW - 6, kTabBarH / 2 - 2, 2, theme.accent);
	}
}

void drawFooter(Canvas& canvas, const BrowserState& state) {
	const Theme& theme = *state.theme;
	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, theme.surface);
	const size_t count = state.view->size();
	const size_t size = pageSize(state.mode);
	const size_t pages = count == 0 ? 1 : (count + size - 1) / size;
	const size_t page = count == 0 ? 1 : pageStart(state.cursor, state.mode) / size + 1;
	const int textY = kHintBarY + (kFooterH - smallFont().height) / 2;
	// Filter on the left and sort order on the right: touching either cycles it (see layout::footer*Rect).
	const std::string filter = filterLabel(state.filter);
	canvas.drawText(smallFont(), 4, textY, filter, state.filter == Filter::All ? theme.muted : theme.accent);
	drawCentered(canvas, smallFont(), {kScreenW * 2 / 5, kHintBarY, kScreenW / 5, kFooterH},
		std::to_string(page) + "/" + std::to_string(pages), theme.muted);
	const std::string sort = sortKeyLabel(state.sort);
	canvas.drawText(smallFont(), kScreenW - 4 - textWidth(smallFont(), sort), textY, sort, theme.muted);
}

void drawGridCell(Canvas& canvas, const BrowserState& state, size_t index) {
	const Theme& theme = *state.theme;
	const GameEntry& game = state.library->games[(*state.view)[index]];
	const Rect cell = gridCellRect(int(index - pageStart(index, ViewMode::Grid)));
	canvas.fillRect(cell, theme.background);
	if (index == state.cursor) {
		canvas.fillRect({cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, theme.surfaceHigh);
		canvas.strokeRect({cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, theme.accent, 2);
	}
	int thumbW = 0, thumbH = 0;
	const uint16_t* thumb = state.thumbs ? state.thumbs->get(game.path, thumbW, thumbH) : nullptr;
	if (thumb) {
		canvas.blit(thumb, thumbW, thumbH, cell.x + (cell.w - thumbW) / 2, cell.y + (cell.h - thumbH) / 2);
	} else {
		drawGameTile(canvas, theme, *state.library, *state.icons, game, cell.x + (cell.w - kIconSize) / 2,
			cell.y + (cell.h - kIconSize) / 2, kIconSize);
	}
	if (isFavorite(state, game)) drawStar(canvas, cell.x + cell.w - 13, cell.y + 4, theme.favorite);
	if (game.portuguese) drawBadge(canvas, cell.x + 3, cell.y + cell.h - smallFont().height - 4, "BR", kBadgeGreen, kBadgeYellow);
}

void drawListRow(Canvas& canvas, const BrowserState& state, size_t index) {
	const Theme& theme = *state.theme;
	const GameEntry& game = state.library->games[(*state.view)[index]];
	const Rect r = listRowRect(int(index - pageStart(index, ViewMode::List)));
	const bool selected = index == state.cursor;
	canvas.fillRect(r, selected ? theme.surfaceHigh : theme.background);
	if (selected) canvas.fillRect({r.x, r.y, 3, r.h}, theme.accent);
	drawGameTile(canvas, theme, *state.library, *state.icons, game, 6, r.y, 16);
	const bool favorite = isFavorite(state, game);
	const int badgeW = game.portuguese ? textWidth(smallFont(), "BR") + 10 : 0;
	const int textW = kScreenW - 28 - (favorite ? 14 : 4) - badgeW;
	canvas.drawText(smallFont(), 28, r.y + (r.h - smallFont().height) / 2, ellipsize(smallFont(), game.title, textW),
		selected ? theme.text : theme.muted);
	if (favorite) drawStar(canvas, kScreenW - 12, r.y + 3, theme.favorite);
	if (game.portuguese) {
		drawBadge(canvas, kScreenW - (favorite ? 14 : 4) - badgeW + 2, r.y + (r.h - smallFont().height - 1) / 2, "BR",
			kBadgeGreen, kBadgeYellow);
	}
}

} // namespace

std::vector<Rect> tabBarRects(const std::vector<Tab>& tabs, Tab active) {
	std::vector<int> widths;
	int activeIndex = -1;
	for (size_t i = 0; i < tabs.size(); ++i) {
		widths.push_back(textWidth(smallFont(), tabLabel(tabs[i])));
		if (tabs[i] == active) activeIndex = int(i);
	}
	return tabRects(widths, activeIndex);
}

void drawGameTile(Canvas& canvas, const Theme& theme, const LibraryData& library, IconCache& icons, const GameEntry& game,
	int x, int y, int size) {
	if (game.iconIndex >= 0) {
		const uint16_t* pixels = icons.get(library.icons, game.iconIndex);
		if (size >= kIconSize) {
			canvas.blit(pixels, kIconSize, kIconSize, x, y, size / kIconSize);
		} else {
			uint16_t half[(kIconSize / 2) * (kIconSize / 2)];
			for (int py = 0; py < kIconSize / 2; ++py) {
				for (int px = 0; px < kIconSize / 2; ++px) half[py * (kIconSize / 2) + px] = pixels[(2 * py) * kIconSize + 2 * px];
			}
			canvas.blit(half, kIconSize / 2, kIconSize / 2, x, y);
		}
		return;
	}

	canvas.fillRect({x, y, size, size}, tileColor(theme, game));
	const std::string initials = initialsFor(game.title);
	const Font& font = size >= kIconSize ? largeFont() : smallFont();
	const int scale = size >= kIconSize ? size / kIconSize : 1;
	const int w = textWidth(font, initials) * scale;
	canvas.drawText(font, x + (size - w) / 2, y + (size - font.height * scale) / 2, initials, kTileText, scale);
}

void drawDetailScreen(Canvas& canvas, const BrowserState& state) {
	const Theme& theme = *state.theme;
	canvas.fill(theme.background);
	canvas.fillRect({0, 0, kScreenW, kHeaderH}, theme.surface);
	const int headerY = (kHeaderH - smallFont().height) / 2;
	// Where the list comes from: tab and filter, or the search.
	std::string where = state.tab.kind == Tab::Kind::All ? "All games" : systemInfo(state.tab.system).name;
	if (state.filter != Filter::All) where += std::string(" \xC2\xB7 ") + filterLabel(state.filter);
	if (!state.genre.empty()) where += " \xC2\xB7 " + std::string(state.genre);
	if (!state.query.empty()) where = "Search: " + std::string(state.query);
	const GameEntry* game = selectedGame(state);
	const std::string position =
		game ? std::to_string(state.cursor + 1) + "/" + std::to_string(state.view->size()) : std::string();
	const int positionX = kScreenW - 4 - textWidth(smallFont(), position);
	canvas.drawText(smallFont(), 4, headerY, ellipsize(smallFont(), where, positionX - 12), theme.accent);
	canvas.drawText(smallFont(), positionX, headerY, position, theme.muted);

	if (!game) {
		const Rect middle = {0, kHeaderH, kScreenW, kHintBarY - kHeaderH};
		drawCentered(canvas, largeFont(), {middle.x, middle.y + middle.h / 2 - 20, middle.w, 20}, "No games here", theme.text);
		drawCentered(canvas, smallFont(), {middle.x, middle.y + middle.h / 2 + 4, middle.w, 14},
			state.filter != Filter::All ? "SELECT changes the filter"
			: state.tab.kind == Tab::Kind::All ? "Add ROMs under sd:/roms" : "Press L/R to change tab", theme.muted);
	} else {
		const Rect art = {8, kHeaderH + 6, 112, 112};
		canvas.fillRect(art, theme.surface);
		if (state.cover) {
			canvas.blit(state.cover->pixels.data(), state.cover->width, state.cover->height,
				art.x + (art.w - state.cover->width) / 2, art.y + (art.h - state.cover->height) / 2);
		} else {
			drawGameTile(canvas, theme, *state.library, *state.icons, *game, art.x + 8, art.y + 8, 96);
		}

		// Title in the large font when it fits three lines, otherwise in the small one.
		const int textX = art.x + art.w + 8;
		const int textW = kScreenW - textX - 6;
		const bool large = wrapText(largeFont(), game->title, textW, 4).size() <= 3;
		const Font& titleFont = large ? largeFont() : smallFont();
		int y = art.y + 2;
		for (const std::string& line : wrapText(titleFont, game->title, textW, large ? 3 : 5)) {
			canvas.drawText(titleFont, textX, y, line, theme.text);
			y += titleFont.height + (large ? 1 : 2);
		}
		// Publisher from the banner, or the developer from metadata.ini; then genre, year and players.
		const std::string& maker = game->publisher.empty() ? game->developer : game->publisher;
		if (!maker.empty()) {
			y += 2;
			for (const std::string& line : wrapText(smallFont(), maker, textW, 2)) {
				canvas.drawText(smallFont(), textX, y, line, theme.muted);
				y += smallFont().height + 1;
			}
		}
		std::string facts = game->genre;
		if (game->year) facts += (facts.empty() ? "" : " \xC2\xB7 ") + std::to_string(game->year);
		if (game->players > 1) facts += (facts.empty() ? "" : " \xC2\xB7 ") + std::string("1-") + std::to_string(game->players) + "P";
		if (!facts.empty()) {
			canvas.drawText(smallFont(), textX, y + 1, ellipsize(smallFont(), facts, textW), theme.accent);
			y += smallFont().height + 2;
		}

		// Badges: Portuguese, favorite, save file.
		y += 5;
		int x = textX;
		if (game->portuguese) x = drawBadge(canvas, x, y, "PT-BR", kBadgeGreen, kBadgeYellow) + 4;
		if (isFavorite(state, *game)) {
			drawStar(canvas, x + 1, y + 2, theme.favorite);
			x += 14;
		}
		if (state.hasSave) drawBadge(canvas, x, y, "SAVE", theme.surfaceHigh, theme.text);

		// Details under the art: console and origin, file, play history.
		const FileTags tags = parseFileTags(game->path);
		std::string origin = systemInfo(game->system).name;
		if (!tags.region.empty()) origin += " \xC2\xB7 " + tags.region;
		if (!tags.languages.empty()) origin += " \xC2\xB7 " + tags.languages;
		std::string file = sizeText(game->fileSize);
		if (!game->gameCode.empty()) file += " \xC2\xB7 " + game->gameCode;
		file += " \xC2\xB7 " + fileName(game->path);
		const GameStats* stats = state.userData->find(game->path);
		std::string played = playedText(stats);
		if (stats && stats->lastPlayed >= kFirstRtcTime) played += " \xC2\xB7 last " + formatDate(stats->lastPlayed);
		int lineY = art.y + art.h + 5;
		for (const std::string& line : {origin, file, played}) {
			canvas.drawText(smallFont(), 8, lineY, ellipsize(smallFont(), line, kScreenW - 16), theme.muted);
			lineY += smallFont().height + 1;
		}
	}

	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, theme.surface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH},
		state.query.empty() ? "A:Play X:Find Y:Fav START:Menu SEL:Filter" : "A:Play X:Find B:Clear search Y:Fav", theme.muted);
}

void drawBrowserScreen(Canvas& canvas, const BrowserState& state) {
	const Theme& theme = *state.theme;
	canvas.fill(theme.background);
	if (state.tabs) drawTabs(canvas, theme, *state.tabs, state.tab);
	if (state.view->empty()) {
		drawCentered(canvas, smallFont(), {0, kContentY, kScreenW, kContentH}, "Nothing in this tab", theme.muted);
	} else {
		const size_t first = pageStart(state.cursor, state.mode);
		const size_t last = std::min(state.view->size(), first + pageSize(state.mode));
		for (size_t index = first; index < last; ++index) drawBrowserItem(canvas, state, index);
	}
	drawFooter(canvas, state);
}

void drawBrowserItem(Canvas& canvas, const BrowserState& state, size_t index) {
	if (index >= state.view->size()) return;
	if (state.mode == ViewMode::Grid) drawGridCell(canvas, state, index);
	else drawListRow(canvas, state, index);
}

void drawSearchScreen(Canvas& canvas, const BrowserState& state) {
	const Theme& theme = *state.theme;
	canvas.fill(theme.background);
	canvas.fillRect({0, 0, kScreenW, kHeaderH}, theme.surface);
	canvas.drawText(smallFont(), 4, (kHeaderH - smallFont().height) / 2, "Search", theme.accent);

	const Rect field = {8, kHeaderH + 8, kScreenW - 16, largeFont().height + 8};
	canvas.fillRect(field, theme.surfaceHigh);
	canvas.fillRect({field.x, field.y + field.h - 2, field.w, 2}, theme.accent);
	const std::string shown = std::string(state.query) + "_";
	const int maxChars = (field.w - 8) / largeFont().width;
	const std::string tail = int(shown.size()) > maxChars ? shown.substr(shown.size() - size_t(maxChars)) : shown;
	canvas.drawText(largeFont(), field.x + 4, field.y + 4, tail, theme.text);

	const size_t count = state.view->size();
	int y = field.y + field.h + 6;
	const std::string summary = count == 1 ? "1 game" : std::to_string(count) + " games";
	canvas.drawText(smallFont(), 8, y, state.query.empty() ? "Type a name" : summary, theme.muted);
	y += smallFont().height + 6;
	for (size_t i = 0; i < count && y + smallFont().height <= kHintBarY - 2; ++i, y += smallFont().height + 3) {
		const GameEntry& game = state.library->games[(*state.view)[i]];
		canvas.drawText(smallFont(), 8, y, ellipsize(smallFont(), game.title, kScreenW - 16), theme.text);
	}

	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, theme.surface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, "A:Type B:Delete START:Done", theme.muted);
}

void drawKeyboardScreen(Canvas& canvas, const Theme& theme, int selectedKey) {
	canvas.fill(theme.background);
	drawCentered(canvas, smallFont(), {0, 0, kScreenW, 22}, "Type part of a game's name", theme.muted);
	const std::vector<Key>& keys = keyboardKeys();
	for (size_t i = 0; i < keys.size(); ++i) {
		const Key& key = keys[i];
		const bool selected = int(i) == selectedKey;
		canvas.fillRect(key.rect, selected ? theme.accent : theme.surfaceHigh);
		const Font& font = key.kind == KeyKind::Char ? largeFont() : smallFont();
		drawCentered(canvas, font, key.rect, key.label, theme.text);
	}
	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, theme.surface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, "Touch or use the D-pad", theme.muted);
}

void drawMenuScreen(Canvas& canvas, const Theme& theme, const std::string& title, const std::vector<MenuItem>& items,
	int selected, const std::string& hint) {
	canvas.fill(theme.background);
	drawCentered(canvas, largeFont(), {0, 3, kScreenW, largeFont().height}, title, theme.text);
	const std::string version = std::string("v") + kVersion;
	canvas.drawText(smallFont(), kScreenW - 6 - textWidth(smallFont(), version), 7, version, theme.muted);
	const int first = menuFirstRow(selected, int(items.size()));
	if (first > 0) drawArrow(canvas, 5, kMenuTop + 2, -1, theme.muted);
	if (first + kMenuVisibleRows < int(items.size())) {
		drawArrow(canvas, 5, kMenuTop + kMenuVisibleRows * kMenuRowH - 8, 1, theme.muted);
	}
	for (size_t i = size_t(first); i < items.size() && int(i) < first + kMenuVisibleRows; ++i) {
		const Rect r = menuRowRect(int(i) - first);
		const bool on = int(i) == selected;
		canvas.fillRect(r, on ? theme.surfaceHigh : theme.surface);
		if (on) canvas.fillRect({r.x, r.y, 3, r.h}, theme.accent);
		const int textY = r.y + (r.h - smallFont().height) / 2;
		canvas.drawText(smallFont(), r.x + 10, textY, items[i].label, on ? theme.text : theme.muted);
		if (!items[i].value.empty()) {
			const std::string value = "< " + items[i].value + " >";
			canvas.drawText(smallFont(), r.x + r.w - 8 - textWidth(smallFont(), value), textY, value,
				on ? theme.accent : theme.muted);
		}
	}
	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, theme.surface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, hint, theme.muted);
}

void drawMessageScreen(Canvas& canvas, const Theme& theme, const std::string& title, const std::vector<std::string>& lines) {
	canvas.fill(theme.background);
	int y = kScreenH / 2 - 10 - int(lines.size()) * 7;
	drawCentered(canvas, largeFont(), {0, y, kScreenW, largeFont().height}, title, theme.text);
	y += largeFont().height + 8;
	for (const std::string& line : lines) {
		drawCentered(canvas, smallFont(), {8, y, kScreenW - 16, smallFont().height}, line, theme.muted);
		y += smallFont().height + 2;
	}
}

} // namespace dscore
