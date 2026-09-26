#include "platform/Power.h"

#include <nds.h>

namespace dscore {

// FIFO values used by the ARM7 core copied from TWiLight's imageview (arm7/source/main.c).
bool powerButtonPressed() {
	return fifoCheckValue32(FIFO_USER_01);
}

void returnToSystemMenu() {
	fifoSendValue32(FIFO_USER_02, 1);
	while (true) swiWaitForVBlank();
}

void sleepWhileLidClosed() {
	if (!(keysHeld() & KEY_LID)) return;
	powerOff(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
	while (keysHeld() & KEY_LID) {
		swiWaitForVBlank();
		scanKeys();
	}
	powerOn(PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM);
}

} // namespace dscore
