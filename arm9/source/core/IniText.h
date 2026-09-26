#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace dscore {

// Trims spaces and tabs.
std::string_view trimIni(std::string_view s);

// Lines without their terminators ("\n" or "\r\n"); a trailing newline adds no empty line.
std::vector<std::string_view> splitIniLines(std::string_view text);

// Calls onEntry(section, key, value) for every "key = value" line, trimmed. A section header is a line
// starting with '[' and ending with ']'; its name is everything in between, so names may contain
// brackets. Lines before the first section have an empty section name.
void forEachIniEntry(std::string_view text,
	const std::function<void(std::string_view section, std::string_view key, std::string_view value)>& onEntry);

// Parses a decimal unsigned value; false unless the whole string is digits and fits in 32 bits.
bool parseIniUint(std::string_view text, uint32_t& out);

} // namespace dscore
