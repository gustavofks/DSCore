#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace dscore {

struct IniKey {
	std::string_view key;
	std::string value;
};

// Sets keys inside [section] of an INI text and preserves every other line byte for byte.
// Existing keys (matched after trimming) are rewritten in place as "KEY = value"; missing keys are
// appended at the end of the section; a missing section is appended at the end of the text.
// Uses "\r\n" when the text contains it, "\n" otherwise, and keeps the final newline as it was.
std::string patchIni(std::string_view text, std::string_view section, const std::vector<IniKey>& keys);

} // namespace dscore
