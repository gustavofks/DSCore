#include "core/IniPatch.h"

namespace dscore {

namespace {

std::string_view trim(std::string_view s) {
	while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
	while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
	return s;
}

// Lines without their terminators; a trailing newline does not produce an empty last line.
std::vector<std::string_view> splitLines(std::string_view text) {
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

std::string keyLine(std::string_view key, const std::string& value) {
	std::string line(key);
	line += " = ";
	line += value;
	return line;
}

} // namespace

std::string patchIni(std::string_view text, std::string_view section, const std::vector<IniKey>& keys) {
	const std::string eol = (text.find("\r\n") != std::string_view::npos) ? "\r\n" : "\n";
	std::vector<bool> done(keys.size(), false);
	std::vector<std::string> out;

	auto appendMissing = [&]() {
		for (size_t i = 0; i < keys.size(); ++i) {
			if (!done[i]) {
				out.push_back(keyLine(keys[i].key, keys[i].value));
				done[i] = true;
			}
		}
	};

	bool inSection = false;
	bool sectionSeen = false;
	for (std::string_view line : splitLines(text)) {
		const std::string_view t = trim(line);
		if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
			if (inSection) appendMissing();
			inSection = (t.substr(1, t.size() - 2) == section);
			sectionSeen = sectionSeen || inSection;
			out.emplace_back(line);
			continue;
		}
		if (inSection) {
			const size_t eq = t.find('=');
			if (eq != std::string_view::npos) {
				const std::string_view key = trim(t.substr(0, eq));
				bool replaced = false;
				for (size_t i = 0; i < keys.size() && !replaced; ++i) {
					if (!done[i] && key == keys[i].key) {
						out.push_back(keyLine(key, keys[i].value));
						done[i] = true;
						replaced = true;
					}
				}
				if (replaced) continue;
			}
		}
		out.emplace_back(line);
	}
	if (inSection) appendMissing();
	if (!sectionSeen) {
		out.push_back("[" + std::string(section) + "]");
		appendMissing();
	}

	std::string result;
	for (size_t i = 0; i < out.size(); ++i) {
		if (i > 0) result += eol;
		result += out[i];
	}
	if (text.empty() || text.back() == '\n') result += eol;
	return result;
}

} // namespace dscore
