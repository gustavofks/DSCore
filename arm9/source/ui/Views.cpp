#include "ui/Views.h"

#include "core/RomMedia.h"
#include "core/Text.h"
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

void drawGrid(Canvas& canvas, const BrowserState& state) {
	const size_t first = pageStart(state.cursor, ViewMode::Grid);
	for (int slot = 0; slot < kGridPerPage; ++slot) {
		const size_t index = first + size_t(slot);
		if (index >= state.view->size()) break;
		const GameEntry& game = state.library->games[(*state.view)[index]];
		const Rect cell = gridCellRect(slot);
		if (index == state.cursor) {
			canvas.fillRect({cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, palette::kSurfaceHigh);
			canvas.strokeRect({cell.x + 2, cell.y + 2, cell.w - 4, cell.h - 4}, palette::kAccent, 2);
		}
		drawGameTile(canvas, *state.library, game, cell.x + (cell.w - kIconSize) / 2, cell.y + (cell.h - kIconSize) / 2,
			kIconSize);
		if (isFavorite(state, game)) drawStar(canvas, cell.x + cell.w - 13, cell.y + 4, palette::kFavorite);
	}
}

void drawList(Canvas& canvas, const BrowserState& state) {
	const size_t first = pageStart(state.cursor, ViewMode::List);
	for (int row = 0; row < kListRows; ++row) {
		const size_t index = first + size_t(row);
		if (index >= state.view->size()) break;
		const GameEntry& game = state.library->games[(*state.view)[index]];
		const Rect r = listRowRect(row);
		if (index == state.cursor) {
			canvas.fillRect(r, palette::kSurfaceHigh);
			canvas.fillRect({r.x, r.y, 3, r.h}, palette::kAccent);
		}
		drawGameTile(canvas, *state.library, game, 6, r.y, 16);
		const bool favorite = isFavorite(state, game);
		const int textW = kScreenW - 28 - (favorite ? 14 : 4);
		canvas.drawText(smallFont(), 28, r.y + (r.h - smallFont().height) / 2, ellipsize(smallFont(), game.title, textW),
			index == state.cursor ? palette::kText : palette::kMuted);
		if (favorite) drawStar(canvas, kScreenW - 12, r.y + 3, palette::kFavorite);
	}
}

} // namespace

void drawGameTile(Canvas& canvas, const LibraryData& library, const GameEntry& game, int x, int y, int size) {
	if (game.iconIndex >= 0) {
		uint16_t pixels[kIconSize * kIconSize];
		decodeNdsIcon(library.icons[size_t(game.iconIndex)], pixels);
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
	canvas.drawText(smallFont(), 4, headerY, "DSCore", palette::kAccent);
	if (state.view && !state.view->empty()) {
		const std::string position = std::to_string(state.cursor + 1) + "/" + std::to_string(state.view->size());
		canvas.drawText(smallFont(), kScreenW - 4 - textWidth(smallFont(), position), headerY, position, palette::kMuted);
	}

	const GameEntry* game = selectedGame(state);
	if (!game) {
		const Rect middle = {0, kHeaderH, kScreenW, kHintBarY - kHeaderH};
		drawCentered(canvas, largeFont(), {middle.x, middle.y + middle.h / 2 - 20, middle.w, 20}, "No games here", palette::kText);
		drawCentered(canvas, smallFont(), {middle.x, middle.y + middle.h / 2 + 4, middle.w, 14},
			state.tab == Tab::All ? "Add ROMs to sd:/roms/NDS or sd:/roms/GBA" : "Press L/R to change tab", palette::kMuted);
	} else {
		canvas.fillRect({12, 28, 104, 104}, palette::kSurface);
		drawGameTile(canvas, *state.library, *game, 16, 32, 96);

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
		if (stats && stats->favorite) {
			drawStar(canvas, textX, y + 2, palette::kFavorite);
			canvas.drawText(smallFont(), textX + 13, y + 1, "Favorite", palette::kFavorite);
		}
		// The file name tells apart dumps with the same title.
		canvas.drawText(smallFont(), 12, 142, ellipsize(smallFont(), fileName(game->path), kScreenW - 24), palette::kMuted);
	}

	canvas.fillRect({0, kHintBarY, kScreenW, kFooterH}, palette::kSurface);
	drawCentered(canvas, smallFont(), {0, kHintBarY, kScreenW, kFooterH}, "A:Play Y:Fav L/R:Tab START:Sort SEL:View",
		palette::kMuted);
}

void drawBrowserScreen(Canvas& canvas, const BrowserState& state) {
	canvas.fill(palette::kBackground);
	drawTabs(canvas, state.tab);
	if (state.view->empty()) {
		drawCentered(canvas, smallFont(), {0, kContentY, kScreenW, kContentH}, "Nothing in this tab", palette::kMuted);
	} else if (state.mode == ViewMode::Grid) {
		drawGrid(canvas, state);
	} else {
		drawList(canvas, state);
	}
	drawFooter(canvas, state);
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
