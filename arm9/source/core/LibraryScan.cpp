#include "core/LibraryScan.h"

#include <set>

namespace dscore {

bool applyScan(LibraryData& library, const std::vector<std::string>& foundPaths, const RomParser& parse) {
	const std::set<std::string> found(foundPaths.begin(), foundPaths.end());
	std::set<std::string> known;
	LibraryData next;
	bool changed = false;

	for (const GameEntry& game : library.games) {
		if (!found.count(game.path)) {
			changed = true;
			continue;
		}
		known.insert(game.path);
		GameEntry kept = game;
		if (game.iconIndex >= 0) {
			kept.iconIndex = int32_t(next.icons.size());
			next.icons.push_back(library.icons[size_t(game.iconIndex)]);
		}
		next.games.push_back(std::move(kept));
	}

	for (const std::string& path : foundPaths) {
		if (known.count(path)) continue;
		known.insert(path);
		changed = true;
		GameEntry game;
		NdsIcon icon{};
		const bool isDs = romKindFor(path) == RomKind::Nds;
		if (!parse(path, game, isDs ? &icon : nullptr)) continue;
		game.path = path;
		game.iconIndex = -1;
		if (isDs && game.system == System::Nds) {
			game.iconIndex = int32_t(next.icons.size());
			next.icons.push_back(icon);
		}
		next.games.push_back(std::move(game));
	}

	library = std::move(next);
	return changed;
}

} // namespace dscore
