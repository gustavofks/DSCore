#include "core/Tone.h"

#include <algorithm>

namespace dscore {

std::vector<int8_t> synthesizeTone(const ToneSpec& spec, int sampleRate) {
	const int count = std::max(1, sampleRate * spec.durationMs / 1000);
	const int attack = std::max(1, count / 20);
	std::vector<int8_t> samples(static_cast<size_t>(count));
	uint32_t phase = 0; // 16.16 fixed-point cycles
	for (int i = 0; i < count; ++i) {
		const int hz = spec.startHz + (spec.endHz - spec.startHz) * i / count;
		phase += uint32_t((int64_t(hz) << 16) / sampleRate);
		const int envelope = i < attack ? i * 256 / attack : (count - i) * 256 / (count - attack);
		const int amplitude = spec.volume * envelope / 256;
		// Soft square: two levels with a short ramp between them avoids harsh edges.
		const int position = int(phase & 0xFFFF);
		int wave;
		if (position < 0x6000) wave = 1;
		else if (position < 0x8000) wave = 1 - 2 * (position - 0x6000) / 0x2000;
		else if (position < 0xE000) wave = -1;
		else wave = -1 + 2 * (position - 0xE000) / 0x2000;
		samples[size_t(i)] = int8_t(std::clamp(wave * amplitude, -127, 127));
	}
	return samples;
}

} // namespace dscore
