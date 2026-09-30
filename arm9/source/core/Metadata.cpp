#include "core/Metadata.h"

#include <algorithm>

#include "core/IniText.h"

namespace dscore {

MetadataMap parseMetadata(std::string_view ini) {
	MetadataMap metadata;
	forEachIniEntry(ini, [&](std::string_view section, std::string_view key, std::string_view value) {
		if (section.empty() || value.empty()) return;
		auto it = metadata.find(section);
		if (it == metadata.end()) it = metadata.emplace(std::string(section), GameMeta{}).first;
		GameMeta& meta = it->second;
		uint32_t number = 0;
		if (key == "genre") meta.genre = std::string(value);
		else if (key == "developer") meta.developer = std::string(value);
		else if (key == "year" && parseIniUint(value, number) && number <= 9999) meta.year = uint16_t(number);
		else if (key == "players" && parseIniUint(value, number) && number <= 255) meta.players = uint8_t(number);
	});
	return metadata;
}

size_t applyMetadata(const MetadataMap& metadata, std::vector<GameEntry>& games) {
	size_t matched = 0;
	for (GameEntry& game : games) {
		const size_t slash = game.path.find_last_of('/');
		const std::string_view name = std::string_view(game.path).substr(slash == std::string::npos ? 0 : slash + 1);
		const auto it = metadata.find(name);
		const GameMeta meta = it == metadata.end() ? GameMeta{} : it->second;
		game.genre = meta.genre;
		game.developer = meta.developer;
		game.year = meta.year;
		game.players = meta.players;
		if (it != metadata.end()) ++matched;
	}
	return matched;
}

std::vector<std::string> genresOf(const std::vector<GameEntry>& games) {
	std::vector<std::string> genres;
	for (const GameEntry& game : games) {
		if (!game.genre.empty()) genres.push_back(game.genre);
	}
	std::sort(genres.begin(), genres.end());
	genres.erase(std::unique(genres.begin(), genres.end()), genres.end());
	return genres;
}

} // namespace dscore
