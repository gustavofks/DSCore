#include "platform/Effects.h"

#include <nds.h>

#include "core/Tone.h"

namespace dscore {

namespace {

constexpr int kSampleRate = 16000;
constexpr int kBlack = -16;
constexpr int kTransitionStart = -6;

constexpr ToneSpec kTones[5] = {
	{0, 0, 0, 0},           // Sound::None
	{1400, 1400, 18, 26},   // Move: short tick
	{880, 1320, 45, 42},    // Select: rising blip
	{660, 440, 45, 38},     // Back: falling blip
	{660, 1320, 140, 48},   // Launch: longer rising chirp
};

// Main engine = top screen, sub engine = bottom screen (lcdMainOnTop).
constexpr int kBothScreens = 3;
constexpr int kBottomScreen = 2;

} // namespace

SoundEffects::SoundEffects() {
	// The ARM7 core scales the master volume by this flag (1 fades out, 2 fades in); 0 keeps it full.
	*(vu32*)0x02003004 = 0;
	soundEnable();
	for (int i = 1; i < 5; ++i) {
		clips_[i] = synthesizeTone(kTones[i], kSampleRate);
		DC_FlushRange(clips_[i].data(), clips_[i].size()); // the sound hardware reads main RAM directly
	}
}

void SoundEffects::play(Sound sound) {
	const std::vector<int8_t>& clip = clips_[int(sound)];
	if (sound == Sound::None || clip.empty()) return;
	soundPlaySample(clip.data(), SoundFormat_8Bit, u32(clip.size()), kSampleRate, 127, 64, false, 0);
}

void fadeIn(int frames) {
	for (int i = frames; i >= 0; --i) {
		setBrightness(kBothScreens, kBlack * i / frames);
		swiWaitForVBlank();
	}
}

void fadeOut(int frames) {
	for (int i = 0; i <= frames; ++i) {
		setBrightness(kBothScreens, kBlack * i / frames);
		swiWaitForVBlank();
	}
}

void BottomFade::start() {
	level_ = kTransitionStart;
	setBrightness(kBottomScreen, level_);
}

void BottomFade::update() {
	if (level_ == 0) return;
	++level_;
	setBrightness(kBottomScreen, level_);
}

} // namespace dscore
