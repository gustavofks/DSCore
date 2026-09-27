#pragma once

// Marks hot drawing loops. On the DS they run from ITCM in ARM mode, avoiding slow instruction fetches
// from main RAM; host builds (tests, preview) ignore it.
#if defined(ARM9)
#define DSCORE_HOT __attribute__((section(".itcm"), long_call, target("arm")))
#else
#define DSCORE_HOT
#endif
