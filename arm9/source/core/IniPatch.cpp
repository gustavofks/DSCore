#include "core/IniPatch.h"

#include "core/IniText.h"

namespace dscore {

namespace {

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
	for (std::string_view line : splitIniLines(text)) {
		const std::string_view t = trimIni(line);
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
				const std::string_view key = trimIni(t.substr(0, eq));
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
