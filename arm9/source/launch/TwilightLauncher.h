#pragma once

#include <string>

namespace dscore {

enum class LaunchError { None, Unsupported, RomRead, MainRead, SettingsRead, SettingsWrite, LoaderFailed };

// Makes TWiLight Menu++ relaunch romPath: writes the per-launch keys its ROM browser would write to
// settings.ini (backing it up once), sets the auto-run bit plus the warm-relaunch marker the installed
// main.srldr expects, and boots main.srldr.
// Only returns on failure; loaderCode receives runNdsFile's return value when the loader fails.
LaunchError launchViaTwilight(const std::string& romPath, int* loaderCode);

const char* describe(LaunchError error);

} // namespace dscore
