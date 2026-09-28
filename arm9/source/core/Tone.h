#pragma once

#include <cstdint>
#include <vector>

namespace dscore {

// Signed 8-bit PCM for short interface sounds, so no audio files ship with DSCore.
struct ToneSpec {
	int startHz;
	int endHz;      // pitch glides linearly from startHz to endHz
	int durationMs;
	int volume;     // peak amplitude, 1..127
};

// A square-ish tone at sampleRate with a quick attack and a linear fade-out, so it never clicks.
std::vector<int8_t> synthesizeTone(const ToneSpec& spec, int sampleRate);

} // namespace dscore
