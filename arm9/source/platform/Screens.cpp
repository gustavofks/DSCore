#include "platform/Screens.h"

#include <nds.h>

#include "ui/Layout.h"

namespace dscore {

namespace {

constexpr size_t kPixels = size_t(layout::kScreenW) * layout::kScreenH;

uint16_t topBuffer[kPixels] __attribute__((aligned(32)));
uint16_t bottomBuffer[kPixels] __attribute__((aligned(32)));

} // namespace

Screens::Screens()
	: top_(topBuffer, layout::kScreenW, layout::kScreenH), bottom_(bottomBuffer, layout::kScreenW, layout::kScreenH) {
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);
	topVram_ = bgGetGfxPtr(bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0));
	bottomVram_ = bgGetGfxPtr(bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0));
	lcdMainOnTop();
}

void Screens::present() {
	DC_FlushRange(topBuffer, sizeof(topBuffer));
	DC_FlushRange(bottomBuffer, sizeof(bottomBuffer));
	swiWaitForVBlank();
	// The 256x256 bitmaps have the same row stride as the 256-pixel-wide buffers.
	dmaCopy(topBuffer, topVram_, sizeof(topBuffer));
	dmaCopy(bottomBuffer, bottomVram_, sizeof(bottomBuffer));
}

} // namespace dscore
