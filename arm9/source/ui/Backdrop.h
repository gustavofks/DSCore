#pragma once

#include <cstdint>

#include "core/Cover.h"

namespace dscore {

constexpr int kBackdropW = 256;
constexpr int kBackdropH = 192;

// Full-screen blurred version of a cover for the details screen: the cover shrunk to a few pixels,
// stretched back with bilinear filtering, darkened towards base (more at the bottom) and dithered so the
// DS's 32 levels per channel show no bands. Integer math only; the ARM9 has no FPU.
// out receives kBackdropW * kBackdropH colors.
void buildBackdrop(const Cover& cover, uint16_t base, uint16_t* out);

} // namespace dscore
