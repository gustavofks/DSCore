#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace dscore {

// Converts little-endian UTF-16 to UTF-8, stopping at the first NUL or after maxUnits code units.
// Lone surrogates become U+FFFD.
std::string utf16leToUtf8(const uint8_t* data, size_t maxUnits);

// Case-insensitive ASCII suffix match that requires at least one character before the extension.
bool hasExtension(std::string_view name, std::string_view ext);

// Replaces every non-ASCII character (a whole UTF-8 sequence) with '?', for the libnds text console.
std::string asciiForConsole(std::string_view utf8);

} // namespace dscore
