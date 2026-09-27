#include "ui/Views.h"

#include <algorithm>

#include "core/RomMedia.h"
#include "core/Text.h"
#include "ui/Keyboard.h"
#include "ui/Layout.h"
#include "ui/Navigation.h"
#include "ui/Palette.h"

namespace dscore {

namespace {

using namespace layout;

constexpr int kHeaderH = 16;
constexpr int kHintBarY = kScreenH - kFooterH;
constexpr uint32_t kFirstRtcTime = 1000000000; // below this, lastPlayed is an imported rank, not a date
constexpr const char* kTabLabels[kTabCount] = {"All", "Fav", "DS", "GBA", "Recent"};

// 9x9 star, bit 8 = leftmost pixel.
constexpr uint16_t kStar[9] = {0x010, 0x010, 0x038, 0x1FF, 0x0FE, 0x07C, 0x06C, 0x0C6, 0x082};

void drawStar(Canvas& canvas, int x, int y, uint16_t color) {
	for (int row = 0; row < 9; ++row) {
		for (int col = 0; col < 9; ++col) {
			if (kStar[row] & (0x100 >> col)) canvas.fillRect({x + col, y + row, 1, 1}, color);
		}
	}
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

std::string systemName(System system) {
	return system == System::Nds ? "Nintendo DS" : "Game Boy Advance";
}

std::string sizeText(uint32_t bytes) {
	if (bytes >= (1u << 20)) return std::to_string(bytes >> 20) + " MB";
	return std::to_string(bytes >> 10) + " KB";
}

uint16_t tileColor(const GameEntry& game) {
	uint32_t hash = 2166136261u;
	for (char c : game.title) hash = (hash ^ uint8_t(c)) * 16777619u;
	const uint16_t* shades = game.system == System::Nds ? palette::kNdsShades : palette::kGbaShades;
	return shades[hash % 4];
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

void drawTabs(Canvas& canvas, Tab active) {
	for (int i = 0; i < kTabCount; ++i) {
		const Rect r = tabRect(i);
		const bool on = i == int(active);
		canvas.fillRect({r.x + 1, r.y, r.w - 2, r.h}, on ? palette::kAccent : palette::kSurface);
		drawCentered(canvas, smallFont(), r, kTabLabels[i], on ? palette::kText : palette::kMuted);
	}
}

void drawFooter(Canvas& canvas, const BrowserState& state) {
	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, palette::kSurface);
	const size_t count = state.view->size();
	const size_t size = pageSize(state.mode);
	const size_t pages = count == 0 ? 1 : (count + size - 1) / size;
	const size_t page = count == 0 ? 1 : pageStart(state.cursor, state.mode) / size + 1;
	const int textY = kHintBarY + (kFooterH - smallFont().height) / 2;
	canvas.drawText(smallFont(), 4, textY, "Page " + std::to_string(page) + "/" + std::to_string(pages), palette::kMuted);

	const std::string sort = state.tab == Tab::Recent ? "Newest first" : std::string("Sort: ") + sortKeyLabel(state.sort);
	canvas.drawText(smallFont(), kScreenW - 4 - textWidth(smallFont(), sort), textY, sort, palette::kMuted);
}

void drawGridCell(Canvas& canvas, const BrowserState& state, size_t index) {
	const GameEntry& game = state.library->games[(*state.view)[index]];
	const Rect cell = gridCellRect(int(index - pageStart(index, ViewMode::Grid)));
	canvas.fillRect(cell, palette::kBackground);
	if (index == state.cursor) {
		canvas.fillRect({cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, palette::kSurfaceHigh);
		canvas.strokeRect({cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, palette::kAccent, 2);
	}
	drawGameTile(canvas, *state.library, *state.icons, game, cell.x + (cell.w - kIconSize) / 2,
		cell.y + (cell.h - kIconSize) / 2, kIconSize);
	if (isFavorite(state, game)) drawStar(canvas, cell.x + cell.w - 13, cell.y + 4, palette::kFavorite);
}

void drawListRow(Canvas& canvas, const BrowserState& state, size_t index) {
	const GameEntry& game = state.library->games[(*state.view)[index]];
	const Rect r = listRowRect(int(index - pageStart(index, ViewMode::List)));
	const bool selected = index == state.cursor;
	canvas.fillRect(r, selected ? palette::kSurfaceHigh : palette::kBackground);
	if (selected) canvas.fillRect({r.x, r.y, 3, r.h}, palette::kAccent);
	drawGameTile(canvas, *state.library, *state.icons, game, 6, r.y, 16);
	const bool favorite = isFavorite(state, game);
	const int textW = kScreenW - 28 - (favorite ? 14 : 4);
	canvas.drawText(smallFont(), 28, r.y + (r.h - smallFont().height) / 2, ellipsize(smallFont(), game.title, textW),
		selected ? palette::kText : palette::kMuted);
	if (favorite) drawStar(canvas, kScreenW - 12, r.y + 3, palette::kFavorite);
}

} // namespace

void drawGameTile(Canvas& canvas, const LibraryData& library, IconCache& icons, const GameEntry& game, int x, int y,
	int size) {
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

	canvas.fillRect({x, y, size, size}, tileColor(game));
	const std::string initials = initialsFor(game.title);
	const Font& font = size >= kIconSize ? largeFont() : smallFont();
	const int scale = size >= kIconSize ? size / kIconSize : 1;
	const int w = textWidth(font, initials) * scale;
	canvas.drawText(font, x + (size - w) / 2, y + (size - font.height * scale) / 2, initials, palette::kText, scale);
}

void drawDetailScreen(Canvas& canvas, const BrowserState& state) {
	canvas.fill(palette::kBackground);
	canvas.fillRect({0, 0, kScreenW, kHeaderH}, palette::kSurface);
	const int headerY = (kHeaderH - smallFont().height) / 2;
	const std::string title = state.query.empty() ? "DSCore" : "Search: " + std::string(state.query);
	canvas.drawText(smallFont(), 4, headerY, ellipsize(smallFont(), title, kScreenW / 2), palette::kAccent);
	const GameEntry* game = selectedGame(state);
	if (game) {
		const std::string position = std::to_string(state.cursor + 1) + "/" + std::to_string(state.view->size());
		const int positionX = kScreenW - 4 - textWidth(smallFont(), position);
		canvas.drawText(smallFont(), positionX, headerY, position, palette::kMuted);
		if (isFavorite(state, *game)) drawStar(canvas, positionX - 14, (kHeaderH - 9) / 2, palette::kFavorite);
	}

	if (!game) {
		const Rect middle = {0, kHeaderH, kScreenW, kHintBarY - kHeaderH};
		drawCentered(canvas, largeFont(), {middle.x, middle.y + middle.h / 2 - 20, middle.w, 20}, "No games here", palette::kText);
		drawCentered(canvas, smallFont(), {middle.x, middle.y + middle.h / 2 + 4, middle.w, 14},
			state.tab == Tab::All ? "Add ROMs to sd:/roms/NDS or sd:/roms/GBA" : "Press L/R to change tab", palette::kMuted);
	} else {
		const Rect art = {8, 24, 112, 112};
		canvas.fillRect(art, palette::kSurface);
		if (state.cover) {
			canvas.blit(state.cover->pixels.data(), state.cover->width, state.cover->height,
				art.x + (art.w - state.cover->width) / 2, art.y + (art.h - state.cover->height) / 2);
		} else {
			drawGameTile(canvas, *state.library, *state.icons, *game, art.x + 8, art.y + 8, 96);
		}

		const int textX = 126;
		const int textW = kScreenW - textX - 8;
		int y = 30;
		for (const std::string& line : wrapText(largeFont(), game->title, textW, 3)) {
			canvas.drawText(largeFont(), textX, y, line, palette::kText);
			y += largeFont().height + 2;
		}
		y += 6;
		const GameStats* stats = state.userData->find(game->path);
		std::string sizeLine = sizeText(game->fileSize);
		if (!game->gameCode.empty()) sizeLine += "  \xC2\xB7  " + game->gameCode; // U+00B7 middle dot
		std::vector<std::string> info = {systemName(game->system), sizeLine, playedText(stats)};
		if (stats && stats->lastPlayed >= kFirstRtcTime) info.push_back("Last: " + formatDate(stats->lastPlayed));
		for (const std::string& line : info) {
			canvas.drawText(smallFont(), textX, y, ellipsize(smallFont(), line, textW), palette::kMuted);
			y += smallFont().height + 2;
		}
		// The file name tells apart dumps with the same title.
		canvas.drawText(smallFont(), 8, kHintBarY - smallFont().height - 4, ellipsize(smallFont(), fileName(game->path), kScreenW - 16),
			palette::kMuted);
	}

	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, palette::kSurface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, state.query.empty() ? "A:Play X:Find Y:Fav START:Sort SEL:View" : "A:Play X:Find B:Clear search Y:Fav",
		palette::kMuted);
}

void drawBrowserScreen(Canvas& canvas, const BrowserState& state) {
	canvas.fill(palette::kBackground);
	drawTabs(canvas, state.tab);
	if (state.view->empty()) {
		drawCentered(canvas, smallFont(), {0, kContentY, kScreenW, kContentH}, "Nothing in this tab", palette::kMuted);
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
	canvas.fill(palette::kBackground);
	canvas.fillRect({0, 0, kScreenW, kHeaderH}, palette::kSurface);
	canvas.drawText(smallFont(), 4, (kHeaderH - smallFont().height) / 2, "Search", palette::kAccent);

	const Rect field = {8, kHeaderH + 8, kScreenW - 16, largeFont().height + 8};
	canvas.fillRect(field, palette::kSurfaceHigh);
	canvas.fillRect({field.x, field.y + field.h - 2, field.w, 2}, palette::kAccent);
	const std::string shown = std::string(state.query) + "_";
	const int maxChars = (field.w - 8) / largeFont().width;
	const std::string tail = int(shown.size()) > maxChars ? shown.substr(shown.size() - size_t(maxChars)) : shown;
	canvas.drawText(largeFont(), field.x + 4, field.y + 4, tail, palette::kText);

	const size_t count = state.view->size();
	int y = field.y + field.h + 6;
	const std::string summary = count == 1 ? "1 game" : std::to_string(count) + " games";
	canvas.drawText(smallFont(), 8, y, state.query.empty() ? "Type a name" : summary, palette::kMuted);
	y += smallFont().height + 6;
	for (size_t i = 0; i < count && y + smallFont().height <= kHintBarY - 2; ++i, y += smallFont().height + 3) {
		const GameEntry& game = state.library->games[(*state.view)[i]];
		canvas.drawText(smallFont(), 8, y, ellipsize(smallFont(), game.title, kScreenW - 16), palette::kText);
	}

	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, palette::kSurface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, "A:Type B:Delete START:Done", palette::kMuted);
}

void drawKeyboardScreen(Canvas& canvas, int selectedKey) {
	canvas.fill(palette::kBackground);
	drawCentered(canvas, smallFont(), {0, 0, kScreenW, 22}, "Type part of a game's name", palette::kMuted);
	const std::vector<Key>& keys = keyboardKeys();
	for (size_t i = 0; i < keys.size(); ++i) {
		const Key& key = keys[i];
		const bool selected = int(i) == selectedKey;
		canvas.fillRect(key.rect, selected ? palette::kAccent : palette::kSurfaceHigh);
		const Font& font = key.kind == KeyKind::Char ? largeFont() : smallFont();
		drawCentered(canvas, font, key.rect, key.label, palette::kText);
	}
	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, palette::kSurface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, "Touch or use the D-pad", palette::kMuted);
}

void drawMessageScreen(Canvas& canvas, const std::string& title, const std::vector<std::string>& lines) {
	canvas.fill(palette::kBackground);
	int y = kScreenH / 2 - 10 - int(lines.size()) * 7;
	drawCentered(canvas, largeFont(), {0, y, kScreenW, largeFont().height}, title, palette::kText);
	y += largeFont().height + 8;
	for (const std::string& line : lines) {
		drawCentered(canvas, smallFont(), {8, y, kScreenW - 16, smallFont().height}, line, palette::kMuted);
		y += smallFont().height + 2;
	}
}

} // namespace dscore
