#include "core/Systems.h"

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

} // namespace

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
