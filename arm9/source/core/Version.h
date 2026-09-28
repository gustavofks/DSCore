#pragma once

// Set by the Makefile from the VERSION file; host builds (tests, preview) show "dev".
#ifndef DSCORE_VERSION
#define DSCORE_VERSION "dev"
#endif

namespace dscore {

constexpr const char* kVersion = DSCORE_VERSION;

} // namespace dscore
