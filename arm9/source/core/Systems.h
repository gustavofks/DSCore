#pragma once

#include <cstdint>
#include <string_view>

namespace dscore {

// Consoles a game can belong to, in tab and sort order. Values are stored in the library cache: append
// new ones, never reorder.
enum class System : uint8_t { Nds, Gba, Gb, Gbc, Nes, Snes, Sms, GameGear, MegaDrive };

constexpr int kSystemCount = int(System::MegaDrive) + 1;

struct SystemInfo {
	const char* id;    // stable lowercase key for config files, e.g. "gba"
	const char* label; // short tab label, e.g. "GBA"
	const char* name;  // full name, e.g. "Game Boy Advance"
};

const SystemInfo& systemInfo(System system);

// The system whose id is id, if any.
bool systemFromId(std::string_view id, System& out);

} // namespace dscore
