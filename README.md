# DSCore
Custom game library frontend for Nintendo DSi, designed to work alongside TWiLight Menu++.

> Status: early development. Tested on a DSi with TWiLight Menu++ v25.10.0.

DSCore is the interface you see. TWiLight Menu++ stays installed as an invisible engine: it boots the
system and starts games through nds-bootstrap and GBARunner2, and DSCore comes back when you quit a
game. Nothing in the system firmware or NAND is touched; everything lives on the SD card and can be
removed.

## Features

- Library of DS (`.nds`) and GBA (`.gba`) games from `sd:/roms/NDS` and `sd:/roms/GBA`, subfolders
  included, with banner icons and titles for DS games and generated tiles for GBA games.
- Grid or list view, tabs (All, Favorites, DS, GBA, Recent) and sorting by name, system or play count.
- Favorites and play history, seeded from TWiLight Menu++'s history on the first run.
- Cached library: after the first indexing only new or removed files are processed.

## Controls

| Input | Action |
|---|---|
| D-pad | Move |
| A | Play |
| Y | Add to / remove from favorites |
| L / R | Previous / next tab |
| START | Change sort order |
| SELECT | Switch between grid and list |
| Touch | Tap a game to select it, tap it again to play; tap a tab to open it |
| Power button | Return to the system menu |

## Installation

Requirements: a DSi with Unlaunch and TWiLight Menu++ installed on the SD card, using one of the DSi,
3DS, Saturn or HBL themes. Back up your SD card first.

1. Build `dscore.nds` (see below) or download it from a release.
2. Copy it over `sd:/_nds/TWiLightMenu/dsimenu.srldr`, keeping the original file as a backup.
   `tools\deploy.ps1 -Target E:\ -Mode srldr` does this and keeps the original as
   `dsimenu.srldr.dscore-orig`.
3. Put your games in `sd:/roms/NDS` and `sd:/roms/GBA`.

A TWiLight Menu++ update replaces `dsimenu.srldr`; copy DSCore again afterwards.

To try DSCore without installing it, run `tools\deploy.ps1 -Target E:\ -Mode app` and open
`dscore.nds` from TWiLight Menu++'s file browser.

## Removal

`tools\restore.ps1 -Target E:\` puts the original `dsimenu.srldr` back. Add `-Settings` to also restore
TWiLight's `settings.ini` and `nds-bootstrap.ini` from the backups DSCore made before first changing
them, and `-Data` to delete DSCore's own files in `sd:/_nds/DSCore`.

## Files on the SD card

| Path | Content |
|---|---|
| `sd:/_nds/DSCore/library.bin` | Library cache (safe to delete; rebuilt on the next start) |
| `sd:/_nds/DSCore/userdata.ini` | Favorites and play history |
| `sd:/_nds/DSCore/config.ini` | Last tab, sort order, view and selected game |
| `sd:/_nds/DSCore/log.txt` | Timings of the last start |

To start a game, DSCore writes the game to TWiLight's `settings.ini` (and, for GBA games,
`nds-bootstrap.ini`) the same way TWiLight's own menu does, then hands over to TWiLight.

## Building

Builds run in Docker with the same devkitARM image TWiLight Menu++ uses. From PowerShell:

```powershell
tools\dk.ps1 make            # builds dscore.nds
tools\dk.ps1 make -C tests   # runs the host unit tests
```

## License

GPL-3.0. The project reuses code from [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)
(GPL-3.0), kept in `third_party/twilight/`, and the [Spleen](https://github.com/fcambus/spleen) font
(BSD-2-Clause), kept in `third_party/spleen/`.
