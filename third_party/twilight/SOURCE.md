# Third-party code: TWiLight Menu++

- Origin: https://github.com/DS-Homebrew/TWiLightMenu
- Tag: `v27.24.1` (commit `68d04c1a621a8d330e7233efcfcb93c94b30a3a6`)
- License: GPL-3.0 (see `LICENSE` in this folder)

## What was copied

| Destination | Origin | Changes |
|---|---|---|
| `third_party/twilight/universal/` | `universal/` | None |
| `arm7/source/main.c` | `imageview/arm7/source/main.c` | None |
| `icon.bmp` | `imageview/icon.bmp` | None (placeholder icon) |

`universal/` provides the chainloader (`runNdsFile`, `bootloader_app`, `bootstub`), DSi SD access,
system details and INI helpers. The ARM7 core is the one TWiLight modules use; the ARM9 system
details code expects its FIFO messages.

## Why v27.24.1

It builds with `devkitpro/devkitarm:20241104`, the image TWiLight Menu++ pins. The loader is linked into
DSCore and only boots the installed `main.srldr`, so the copied version does not have to match the
TWiLight Menu++ version on the SD card.

## Updating

Copy the same paths from a newer tag, update this file, then run the host tests and the hardware
checklist again.
