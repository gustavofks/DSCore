#include "core/UserData.h"

#include <algorithm>
#include <vector>

#include "core/IniText.h"

namespace dscore {

namespace {

std::string joinPath(std::string_view folder, std::string_view file) {
	std::string path(folder);
	if (path.empty() || path.back() != '/') path += '/';
	path += file;
	return path;
}

std::vector<std::string_view> splitColon(std::string_view list) {
	std::vector<std::string_view> items;
	size_t pos = 0;
	while (pos <= list.size()) {
		const size_t colon = list.find(':', pos);
		const size_t end = (colon == std::string_view::npos) ? list.size() : colon;
		if (end > pos) items.push_back(list.substr(pos, end - pos));
		if (colon == std::string_view::npos) break;
		pos = colon + 1;
	}
	return items;
}

} // namespace

const GameStats* UserData::find(std::string_view path) const {
	const auto it = stats_.find(path);
	return it == stats_.end() ? nullptr : &it->second;
}

void UserData::toggleFavorite(const std::string& path) {
	GameStats& stats = stats_[path];
	stats.favorite = !stats.favorite;
}

void UserData::recordLaunch(const std::string& path, uint32_t now) {
	const uint32_t lastPlayed = std::max(now, newestLastPlayed() + 1);
	GameStats& stats = stats_[path];
	++stats.timesPlayed;
	stats.lastPlayed = lastPlayed;
}

void UserData::importTwilightHistory(std::string_view recentIni, std::string_view timesIni) {
	// Each folder lists its files newest first; the longest list sets the numbering so position 0 of
	// every folder gets the largest value.
	std::vector<std::pair<std::string_view, std::vector<std::string_view>>> recent;
	size_t longest = 0;
	forEachIniEntry(recentIni, [&](std::string_view section, std::string_view folder, std::string_view files) {
		if (section != "RECENT") return;
		recent.emplace_back(folder, splitColon(files));
		longest = std::max(longest, recent.back().second.size());
	});
	for (const auto& [folder, files] : recent) {
		for (size_t i = 0; i < files.size(); ++i) {
			GameStats& stats = stats_[joinPath(folder, files[i])];
			stats.lastPlayed = std::max(stats.lastPlayed, uint32_t(longest - i));
		}
	}

	forEachIniEntry(timesIni, [&](std::string_view folder, std::string_view file, std::string_view count) {
		uint32_t times = 0;
		if (folder.empty() || !parseIniUint(count, times)) return;
		GameStats& stats = stats_[joinPath(folder, file)];
		stats.timesPlayed = std::max(stats.timesPlayed, times);
	});
}

std::string UserData::serialize() const {
	std::string out;
	for (const auto& [path, stats] : stats_) {
		if (!stats.favorite && stats.timesPlayed == 0 && stats.lastPlayed == 0) continue;
		out += "[" + path + "]\n";
		if (stats.favorite) out += "favorite = 1\n";
		if (stats.timesPlayed) out += "played = " + std::to_string(stats.timesPlayed) + "\n";
		if (stats.lastPlayed) out += "last = " + std::to_string(stats.lastPlayed) + "\n";
	}
	return out;
}

UserData UserData::parse(std::string_view ini) {
	UserData data;
	forEachIniEntry(ini, [&](std::string_view path, std::string_view key, std::string_view value) {
		if (path.empty()) return;
		uint32_t number = 0;
		if (!parseIniUint(value, number)) return;
		GameStats& stats = data.stats_[std::string(path)];
		if (key == "favorite") stats.favorite = number != 0;
		else if (key == "played") stats.timesPlayed = number;
		else if (key == "last") stats.lastPlayed = number;
	});
	return data;
}

uint32_t UserData::newestLastPlayed() const {
	uint32_t newest = 0;
	for (const auto& entry : stats_) newest = std::max(newest, entry.second.lastPlayed);
	return newest;
}

} // namespace dscore
