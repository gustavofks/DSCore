#include <nds.h>
#include <cstdio>

#include "common/systemdetails.h"
#include "my_gurumeditation.h"

// Read by TWiLight's twlmenusettings.cpp; normally defined in universal/arm9/source/mainAll.cpp.
bool useTwlCfg = false;

int main(int argc, char** argv) {
	myExceptionHandler();
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);
	sys().initFilesystem(argc > 0 ? argv[0] : "sd:/dscore.nds");
	sys().initArm7RegStatuses();

	consoleDemoInit();
	iprintf("DSCore hello\n\n");
	iprintf("DSi mode: %s\n", isDSiMode() ? "yes" : "no");
	iprintf("FAT: %s\n", sys().fatInitOk() ? "ok" : "failed");
	iprintf("Running from SD: %s\n", sys().isRunFromSD() ? "yes" : "no");

	while (true) swiWaitForVBlank();
}
