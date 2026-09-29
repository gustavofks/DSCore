#include "core/Systems.h"

#include "core/Text.h"

namespace dscore {

namespace {

constexpr SystemInfo kSystems[kSystemCount] = {
	{"nds", "DS", "Nintendo DS"},
	{"gba", "GBA", "Game Boy Advance"},
	{"gb", "GB", "Game Boy"},
	{"gbc", "GBC", "Game Boy Color"},
	{"nes", "NES", "Nintendo (NES)"},
	{"snes", "SNES", "Super Nintendo"},
	{"sms", "SMS", "Master System"},
	{"gg", "GG", "Game Gear"},
	{"md", "MD", "Mega Drive"},
};

struct Extension {
	const char* ext;
	System system;
};

// The extensions TWiLight Menu++ v25.10.0's ROM browser hands to each emulator.
constexpr Extension kExtensions[] = {
	{".nds", System::Nds}, {".gba", System::Gba}, {".gb", System::Gb}, {".sgb", System::Gb}, {".gbc", System::Gbc},
	{".nes", System::Nes}, {".fds", System::Nes}, {".sms", System::Sms}, {".gg", System::GameGear},
};

} // namespace

bool systemForPath(std::string_view path, System& out) {
	for (const Extension& e : kExtensions) {
		if (hasExtension(path, e.ext)) {
			out = e.system;
			return true;
		}
	}
	return false;
}

const SystemInfo& systemInfo(System system) {
	return kSystems[int(system) < kSystemCount ? int(system) : 0];
}

bool systemFromId(std::string_view id, System& out) {
	for (int i = 0; i < kSystemCount; ++i) {
		if (id == kSystems[i].id) {
			out = System(i);
			return true;
		}
	}
	return false;
}

} // namespace dscore
