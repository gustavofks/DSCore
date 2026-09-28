#include "doctest.h"

#include <cstdlib>

#include "core/Tone.h"

using namespace dscore;

TEST_CASE("synthesizeTone has the requested length and stays within volume") {
	const auto samples = synthesizeTone({880, 1320, 50, 60}, 16000);
	CHECK(samples.size() == 800);
	int peak = 0;
	for (int8_t s : samples) peak = std::max(peak, std::abs(int(s)));
	CHECK(peak > 30);
	CHECK(peak <= 60);
}

TEST_CASE("synthesizeTone fades in and out so it never clicks") {
	const auto samples = synthesizeTone({440, 440, 40, 100}, 16000);
	CHECK(std::abs(int(samples.front())) <= 5);
	CHECK(std::abs(int(samples.back())) <= 5);
}

TEST_CASE("synthesizeTone always returns at least one sample") {
	CHECK(synthesizeTone({440, 440, 0, 50}, 16000).size() == 1);
}
