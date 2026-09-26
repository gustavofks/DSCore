#include "core/IniText.h"

namespace dscore {

std::string_view trimIni(std::string_view s) {
	while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
	while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
	return s;
}

std::vector<std::string_view> splitIniLines(std::string_view text) {
	std::vector<std::string_view> lines;
	size_t pos = 0;
	while (pos < text.size()) {
		const size_t newline = text.find('\n', pos);
		size_t end = (newline == std::string_view::npos) ? text.size() : newline;
		if (end > pos && text[end - 1] == '\r') --end;
		lines.push_back(text.substr(pos, end - pos));
		pos = (newline == std::string_view::npos) ? text.size() : newline + 1;
	}
	return lines;
}

void forEachIniEntry(std::string_view text,
	const std::function<void(std::string_view section, std::string_view key, std::string_view value)>& onEntry) {
	std::string_view section;
	for (std::string_view line : splitIniLines(text)) {
		const std::string_view t = trimIni(line);
		if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
			section = t.substr(1, t.size() - 2);
			continue;
		}
		const size_t eq = t.find('=');
		if (eq == std::string_view::npos) continue;
		onEntry(section, trimIni(t.substr(0, eq)), trimIni(t.substr(eq + 1)));
	}
}

bool parseIniUint(std::string_view text, uint32_t& out) {
	if (text.empty() || text.size() > 10) return false;
	uint64_t value = 0;
	for (char c : text) {
		if (c < '0' || c > '9') return false;
		value = value * 10 + uint64_t(c - '0');
	}
	if (value > UINT32_MAX) return false;
	out = uint32_t(value);
	return true;
}

} // namespace dscore
