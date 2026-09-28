# DSCore
Custom game library frontend for Nintendo DSi, designed to work alongside TWiLight Menu++.

> Status: early development. Tested on a DSi with TWiLight Menu++ v25.10.0.

DSCore is the interface you see. TWiLight Menu++ stays installed as an invisible engine: it boots the
system and starts games through nds-bootstrap and GBARunner2, and DSCore comes back when you quit a
game. Nothing in the system firmware or NAND is touched; everything lives on the SD card and can be
removed.

## Features

- Library of games found anywhere under `sd:/roms`, recognized by extension: DS (`.nds`), GBA
  (`.gba`), Game Boy (`.gb`, `.sgb`), Game Boy Color (`.gbc`), NES (`.nes`, `.fds`), Master System
  (`.sms`) and Game Gear (`.gg`). DS games show their banner icon and title; the others get a
  generated tile and the file name as title. Games of the other consoles run in the emulators
  TWiLight Menu++ ships in `sd:/_nds/TWiLightMenu/emulators`.
- Box art on the details screen, downloaded on a PC with `tools/fetch_covers.py`.
- Grid or list view, tabs (All, Favorites, one per console with at least one game, Recent) and sorting by name, system or play count.
- Search by name with an on-screen keyboard.
- Favorites and play history, seeded from TWiLight Menu++'s history on the first run.
- Themes: four built in, plus your own color themes as INI files.
- Cached library: after the first indexing only new or removed files are processed.

## Controls

| Input | Action |
|---|---|
| D-pad | Move |
| A | Play |
| X | Search |
| Y | Add to / remove from favorites |
| B | Clear the search |
| L / R | Previous / next tab |
| START | Options: sort order, view, theme, rebuild library |
| SELECT | Switch between grid and list |
| Touch | Tap a game to select it, tap it again to play; tap a tab to open it |
| Power button | Return to the system menu |

While searching, the D-pad and A (or touch) type on the keyboard, B deletes and START finishes.

## Installation

Requirements: a DSi with Unlaunch and TWiLight Menu++ installed on the SD card, using one of the DSi,
3DS, Saturn or HBL themes. Back up your SD card first.

1. Build `dscore.nds` (see below) or download it from a release.
2. Copy it over `sd:/_nds/TWiLightMenu/dsimenu.srldr`, keeping the original file as a backup.
   `tools\deploy.ps1 -Target E:\ -Mode srldr` does this, keeps the original as
   `dsimenu.srldr.dscore-orig` and copies the example themes.
3. Put your games under `sd:/roms`, e.g. `sd:/roms/NDS`, `sd:/roms/GBA`, `sd:/roms/GB`.

A TWiLight Menu++ update replaces `dsimenu.srldr`; copy DSCore again afterwards.

To try DSCore without installing it, run `tools\deploy.ps1 -Target E:\ -Mode app` and open
`dscore.nds` from TWiLight Menu++'s file browser.

## Box art

With the SD card in the PC, run:

```powershell
python tools\fetch_covers.py E:\
```

It identifies each game (by game code for DS and GBA, by CRC32 for the other consoles), downloads its
cover (DS from GameTDB, the rest from libretro-thumbnails), scales it for the DSi and saves it to `sd:/_nds/DSCore/covers`. Only the Python
standard library is needed. Covers already present are skipped; `--force` downloads them again and
`--twilight` also saves the original images where TWiLight Menu++ looks for box art.

## Themes

Pick a theme in the options menu (START). To make your own, put an INI file in
`sd:/_nds/DSCore/themes/`; [themes/Sunset.ini](themes/Sunset.ini) shows every setting:

```ini
[theme]
name = Sunset

[colors]
background = #1A1020
accent = #FF7A45
```

Colors are `#RRGGBB`; anything left out keeps the default theme's value.

## Removal

`tools\restore.ps1 -Target E:\` puts the original `dsimenu.srldr` back. Add `-Settings` to also restore
TWiLight's `settings.ini` and `nds-bootstrap.ini` from the backups DSCore made before first changing
them, and `-Data` to delete DSCore's own files in `sd:/_nds/DSCore`.

## Files on the SD card

| Path | Content |
|---|---|
| `sd:/_nds/DSCore/library.bin` | Library cache (safe to delete; rebuilt on the next start) |
| `sd:/_nds/DSCore/userdata.ini` | Favorites and play history |
| `sd:/_nds/DSCore/config.ini` | Last tab, sort order, view, theme and selected game |
| `sd:/_nds/DSCore/covers/` | Box art from `tools/fetch_covers.py` |
| `sd:/_nds/DSCore/themes/` | Your themes |
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
