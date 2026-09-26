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

namespace {

struct RowSpan {
	bool dirty = false;
	int first = 0;
	int last = 0;
};

RowSpan dirtyRows(Canvas& canvas, uint16_t* buffer) {
	RowSpan span;
	span.dirty = canvas.takeDirtyRows(span.first, span.last);
	if (span.dirty) DC_FlushRange(buffer + span.first * layout::kScreenW, (span.last - span.first) * layout::kScreenW * 2);
	return span;
}

// The 256x256 bitmaps have the same row stride as the 256-pixel-wide buffers.
void copyRows(const RowSpan& span, const uint16_t* buffer, uint16_t* vram) {
	if (!span.dirty) return;
	const size_t offset = size_t(span.first) * layout::kScreenW;
	dmaCopy(buffer + offset, vram + offset, size_t(span.last - span.first) * layout::kScreenW * 2);
}

} // namespace

void Screens::present() {
	const RowSpan top = dirtyRows(top_, topBuffer);
	const RowSpan bottom = dirtyRows(bottom_, bottomBuffer);
	swiWaitForVBlank();
	copyRows(top, topBuffer, topVram_);
	copyRows(bottom, bottomBuffer, bottomVram_);
}

} // namespace dscore
