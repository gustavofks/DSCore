#pragma once

namespace dscore {

// True once the ARM7 core reports the power button (or L+R+START+SELECT).
bool powerButtonPressed();

// Warm-reboots to the system menu, as TWiLight's menus do on the power button; with Unlaunch set to
// autoboot TWiLight this comes back to DSCore.
[[noreturn]] void returnToSystemMenu();

// Turns the backlights off while the lid is closed and back on when it opens.
void sleepWhileLidClosed();

} // namespace dscore
