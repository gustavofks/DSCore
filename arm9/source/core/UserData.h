#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace dscore {

struct GameStats {
	bool favorite = false;
	uint32_t timesPlayed = 0;
	uint32_t lastPlayed = 0; // ordering key: larger is more recent, 0 = never played
};

// Per-game data the user creates (favorites, play history), keyed by ROM path and stored apart from
// the library cache so rebuilding the cache never loses it.
class UserData {
public:
	bool empty() const { return stats_.empty(); }
	const GameStats* find(std::string_view path) const;

	void toggleFavorite(const std::string& path);

	// now is the RTC time; the stored value is bumped past every existing one so the newest launch
	// always sorts first, even with a wrong clock or imported history.
	void recordLaunch(const std::string& path, uint32_t now);

	// Seeds history from TWiLight Menu++'s extras/recentlyplayed.ini and extras/timesplayed.ini.
	// TWiLight keeps no timestamps, so recency becomes small increasing numbers.
	void importTwilightHistory(std::string_view recentIni, std::string_view timesIni);

	// INI text: one [path] section per game with favorite / played / last keys.
	std::string serialize() const;
	static UserData parse(std::string_view ini);

private:
	uint32_t newestLastPlayed() const;

	std::map<std::string, GameStats, std::less<>> stats_;
};

} // namespace dscore
