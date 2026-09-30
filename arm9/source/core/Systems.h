#pragma once

#include <cstdint>
#include <string_view>

namespace dscore {

// Consoles a game can belong to. Values are stored in the library cache: append new ones, never reorder
// (displayOrder() sets the order on screen).
enum class System : uint8_t { Nds, Gba, Gb, Gbc, Nes, Snes, Sms, GameGear, MegaDrive, Atari2600 };

constexpr int kSystemCount = int(System::Atari2600) + 1;

struct SystemInfo {
	const char* id;    // stable lowercase key for config files, e.g. "gba"
	const char* label; // short tab label, e.g. "GBA"
	const char* name;  // full name, e.g. "Game Boy Advance"
};

const SystemInfo& systemInfo(System system);

// Position of system in tabs and in the "System" sort: handhelds first, then home consoles by maker,
// oldest first.
int displayOrder(System system);

// The system of a ROM file DSCore can launch, by extension (ignoring case). False for anything else,
// including consoles whose launch is not supported yet.
bool systemForPath(std::string_view path, System& out);

// The system whose id is id, if any.
bool systemFromId(std::string_view id, System& out);

} // namespace dscore
