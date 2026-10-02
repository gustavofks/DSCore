#include "ui/Views.h"

#include <algorithm>

#include "core/RomMedia.h"
#include "core/Version.h"
#include "core/Text.h"
#include "ui/Backdrop.h"
#include "ui/Keyboard.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"

namespace dscore {

namespace {

using namespace layout;

constexpr int kHeaderH = 16;
constexpr int kMaxArt = 112;   // covers are fitted to 112x112 (tools/fetch_covers.py)
constexpr int kChipRadius = 6;
constexpr int kCardRadius = 5;
constexpr int kHintBarY = kScreenH - kFooterH;
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

uint16_t tileColor(const Theme& theme, const GameEntry& game) {
	uint32_t hash = 2166136261u;
	for (char c : game.title) hash = (hash ^ uint8_t(c)) * 16777619u;
	return tileShades(theme, game.system)[hash % 4];
}

void drawChips(Canvas& canvas, const Theme& theme, const std::vector<Tab>& tabs, Tab active) {
	const std::vector<Rect> rects = tabBarRects(tabs, active);
	for (size_t i = 0; i < tabs.size(); ++i) {
		const Rect& r = rects[i];
		if (r.x >= kScreenW || r.x + r.w <= 0) continue;
		const bool on = tabs[i] == active;
		canvas.fillRounded(r, kChipRadius, on ? theme.accent : theme.surface);
		drawCentered(canvas, smallFont(), r, tabLabel(tabs[i]), on ? readableOn(theme.accent) : theme.muted);
	}
}

// Page as a thin bar; the filter, genre or search on the left and the sort order on the right only when
// they are not the defaults (touching either side still cycles them, see layout::footer*Rect).
void drawFooter(Canvas& canvas, const BrowserState& state) {
	const Theme& theme = *state.theme;
	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, theme.background);
	const int textY = kHintBarY + (kFooterH - smallFont().height) / 2;
	std::string scope;
	if (!state.query.empty()) scope = "Search: " + std::string(state.query);
	if (state.filter != Filter::All) scope += (scope.empty() ? "" : " \xC2\xB7 ") + std::string(filterLabel(state.filter));
	if (!state.genre.empty()) scope += (scope.empty() ? "" : " \xC2\xB7 ") + std::string(state.genre);
	const std::string sort = state.sort == SortKey::Name ? std::string() : sortKeyLabel(state.sort);
	const int sortW = sort.empty() ? 0 : textWidth(smallFont(), sort);
	if (!scope.empty()) canvas.drawText(smallFont(), 6, textY, ellipsize(smallFont(), scope, kScreenW / 2 - 12), theme.accent);
	if (!sort.empty()) canvas.drawText(smallFont(), kScreenW - 6 - sortW, textY, sort, theme.muted);

	const size_t count = state.view->size(), size = pageSize(state.mode);
	const int pages = count == 0 ? 1 : int((count + size - 1) / size);
	if (pages > 1) {
		const Rect track = {kScreenW / 2 - 30, kHintBarY + kFooterH / 2 - 1, 60, 2};
		const int page = int(pageStart(state.cursor, state.mode) / size);
		const int segment = std::max(4, track.w / pages);
		canvas.fillRect(track, theme.surface);
		canvas.fillRect({track.x + page * (track.w - segment) / (pages - 1), track.y, segment, track.h}, theme.muted);
	}
}

void drawGridCell(Canvas& canvas, const BrowserState& state, size_t index) {
	const Theme& theme = *state.theme;
	const GameEntry& game = state.library->games[(*state.view)[index]];
	const Rect cell = gridCellRect(int(index - pageStart(index, ViewMode::Grid)));
	const Rect card = {cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4};
	const bool selected = index == state.cursor;
	const uint16_t cardColor = selected ? theme.accent : theme.surface;
	canvas.fillRect(cell, theme.background);
	canvas.fillRounded(card, kCardRadius, cardColor);
	int thumbW = 0, thumbH = 0;
	const uint16_t* thumb = state.thumbs ? state.thumbs->get(game.path, thumbW, thumbH) : nullptr;
	if (thumb) {
		const Rect art = {card.x + (card.w - thumbW) / 2, card.y + (card.h - thumbH) / 2, thumbW, thumbH};
		canvas.blit(thumb, thumbW, thumbH, art.x, art.y);
		canvas.roundCorners(art, cardColor);
	} else {
		drawGameTile(canvas, theme, *state.library, *state.icons, game, card.x + (card.w - kIconSize) / 2,
			card.y + (card.h - kIconSize) / 2, kIconSize);
	}
	if (isFavorite(state, game)) drawStar(canvas, card.x + card.w - 11, card.y + 2, theme.favorite);
	if (game.portuguese) drawBadge(canvas, card.x + 1, card.y + card.h - smallFont().height - 2, "BR", kBadgeGreen, kBadgeYellow);
}

void drawListRow(Canvas& canvas, const BrowserState& state, size_t index) {
	const Theme& theme = *state.theme;
	const GameEntry& game = state.library->games[(*state.view)[index]];
	const Rect r = listRowRect(int(index - pageStart(index, ViewMode::List)));
	const bool selected = index == state.cursor;
	canvas.fillRect(r, theme.background);
	if (selected) canvas.fillRounded({r.x + 2, r.y, r.w - 4, r.h}, 4, theme.surfaceHigh);
	drawGameTile(canvas, theme, *state.library, *state.icons, game, 8, r.y, 16);
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
	return chipRects(widths, activeIndex);
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
	const GameEntry* game = selectedGame(state);
	if (game && state.backdrop) canvas.blit(state.backdrop, kBackdropW, kBackdropH, 0, 0);
	else canvas.fill(theme.background);

	if (!game) {
		const Rect middle = {0, 0, kScreenW, kHintBarY};
		drawCentered(canvas, largeFont(), {middle.x, middle.y + middle.h / 2 - 20, middle.w, 20}, "No games here", theme.text);
		drawCentered(canvas, smallFont(), {middle.x, middle.y + middle.h / 2 + 4, middle.w, 14},
			state.filter != Filter::All || !state.genre.empty() ? "SELECT or START change the filter"
			: state.tab.kind == Tab::Kind::All ? "Add ROMs under sd:/roms" : "Press L/R to change tab", theme.muted);
	} else {
		// The cover, centred with a shadow; the thumbnail doubled until it loads.
		const Rect art = {(kScreenW - kMaxArt) / 2, 6, kMaxArt, kMaxArt};
		int thumbW = 0, thumbH = 0;
		const uint16_t* thumb = state.thumbs ? state.thumbs->get(game->path, thumbW, thumbH) : nullptr;
		const int w = state.cover ? state.cover->width : thumb ? thumbW * 2 : 96;
		const int h = state.cover ? state.cover->height : thumb ? thumbH * 2 : 96;
		const Rect frame = {art.x + (art.w - w) / 2, art.y + (art.h - h) / 2, w, h};
		canvas.blendRect({frame.x + 3, frame.y + 3, w, h}, rgb(0, 0, 0), 10);
		if (state.cover) canvas.blit(state.cover->pixels.data(), w, h, frame.x, frame.y);
		else if (thumb) canvas.blit(thumb, thumbW, thumbH, frame.x, frame.y, 2);
		else drawGameTile(canvas, theme, *state.library, *state.icons, *game, frame.x, frame.y, 96);

		// Title band: a darker strip under the cover for the name, the facts and the badges.
		const int bandY = art.y + art.h + 4;
		canvas.blendRect({0, bandY, kScreenW, kHintBarY - bandY}, rgb(0, 0, 0), 9);
		int y = bandY + 3;
		const bool oneLine = textWidth(largeFont(), game->title) <= kScreenW - 12;
		if (oneLine) {
			drawCentered(canvas, largeFont(), {0, y, kScreenW, largeFont().height}, game->title, rgb(31, 31, 31));
			y += largeFont().height + 1;
		} else {
			for (const std::string& line : wrapText(smallFont(), game->title, kScreenW - 12, 2)) {
				drawCentered(canvas, smallFont(), {0, y, kScreenW, smallFont().height}, line, rgb(31, 31, 31));
				y += smallFont().height;
			}
			y += 2;
		}
		std::string facts = systemInfo(game->system).name;
		if (game->year) facts += " \xC2\xB7 " + std::to_string(game->year);
		if (!game->genre.empty()) facts += " \xC2\xB7 " + game->genre;
		drawCentered(canvas, smallFont(), {0, y, kScreenW, smallFont().height}, facts, rgb(22, 23, 25));
		y += smallFont().height + 3;

		// Badges, centred: favorite, Portuguese, save file.
		const bool favorite = isFavorite(state, *game);
		const int badgesW = (favorite ? 13 : 0) + (game->portuguese ? textWidth(smallFont(), "PT-BR") + 10 : 0) +
		                    (state.hasSave ? textWidth(smallFont(), "SAVE") + 10 : 0);
		int x = (kScreenW - badgesW) / 2;
		if (favorite && y + 9 <= kHintBarY) {
			drawStar(canvas, x, y + 2, theme.favorite);
			x += 13;
		}
		if (y + 13 <= kHintBarY) {
			if (game->portuguese) x = drawBadge(canvas, x, y, "PT-BR", kBadgeGreen, kBadgeYellow) + 4;
			if (state.hasSave) drawBadge(canvas, x, y, "SAVE", rgb(6, 7, 9), rgb(28, 28, 29));
		}
	}

	canvas.blendRect({0, kHintBarY, kScreenW, kFooterH}, rgb(0, 0, 0), 12);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH},
		state.query.empty() ? "A:Play X:Find Y:Fav START:Menu SEL:Filter" : "A:Play X:Find B:Clear search Y:Fav",
		rgb(17, 18, 21));
}

void drawBrowserScreen(Canvas& canvas, const BrowserState& state) {
	const Theme& theme = *state.theme;
	canvas.fill(theme.background);
	if (state.tabs) drawChips(canvas, theme, *state.tabs, state.tab);
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
