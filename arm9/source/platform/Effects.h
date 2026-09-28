#pragma once

#include <cstdint>
#include <vector>

#include "ui/App.h"

namespace dscore {

// Interface sounds synthesized at startup and played on the hardware sound channels, so they cost
// almost no CPU time.
class SoundEffects {
public:
	SoundEffects();
	void play(Sound sound);

private:
	std::vector<int8_t> clips_[5];
};

// Screen fades done with the hardware master brightness: no drawing involved.
void fadeIn(int frames);  // from black to normal, both screens
void fadeOut(int frames); // from normal to black, both screens

// Brief fade-in of the bottom screen after it switches to another page or tab.
class BottomFade {
public:
	void start();
	void update(); // once per frame

private:
	int level_ = 0; // master brightness, negative = darker
};

} // namespace dscore
