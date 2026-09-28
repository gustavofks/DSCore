#pragma once

#include <string>
#include <string_view>

namespace dscore {

// Lowercase ASCII with Latin-1 accents removed ("Pokémon" -> "pokemon"); other characters unchanged.
std::string foldForSearch(std::string_view text);

// True when query is empty or appears in title, ignoring case and Latin-1 accents.
bool matchesQuery(std::string_view title, std::string_view query);

} // namespace dscore
