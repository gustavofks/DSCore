#pragma once

#include "gfx/Canvas.h"

namespace dscore {

// Both screens as 16-bit bitmap backgrounds, drawn in RAM and copied to VRAM during VBlank.
class Screens {
public:
	Screens();

	Canvas& top() { return top_; }
	Canvas& bottom() { return bottom_; }

	// Waits for VBlank and shows what was drawn since the last call.
	void present();

private:
	Canvas top_;
	Canvas bottom_;
	uint16_t* topVram_;
	uint16_t* bottomVram_;
};

} // namespace dscore
