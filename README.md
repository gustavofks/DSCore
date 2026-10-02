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
  (`.sms`), Game Gear (`.gg`), Super Nintendo (`.sfc`, `.smc`), Mega Drive (`.md`, `.gen`) and Atari 2600 (`.a26`). Games of the
  other consoles run in the emulators TWiLight Menu++ ships in `sd:/_nds/TWiLightMenu/emulators`.
  Mega Drive games named `.gen` and up to 3 MB run in jEnesisDS; `.md` files and larger games run in
  PicoDriveTWL (nds-bootstrap only hands `.gen` files to jEnesisDS). For PicoDriveTWL, DSCore sets
  TWiLight's Mega Drive emulator setting for the launch and puts it back on its next start.
  In Game Boy games, leave GameYob with "Quit to Launcher": its "Exit" opens GameYob's own file list.
- Clean titles: the full DS banner title and publisher, or the file name without region tags,
  release numbers and site names.
- Box art on the details screen and as thumbnails in the grid, downloaded on a PC with
  `tools/fetch_covers.py`.
- Details screen with region, languages, size, play history and badges for Portuguese games,
  favorites and games with a save file.
- Tabs for all games and for each console with at least one game (consoles can be hidden), filters
  (all, favorites, played, not played, Portuguese) and sorting by name, recent, play count or
  system, in a grid or a list.
- Genres, release years and developers from `tools/fetch_metadata.py`, with a genre filter and a
  sort by year.
- Portuguese games are recognized by tags such as `(BR)`, `(PT)` or `(En,Pt)` in the file name, or a
  folder named `br`, and marked with a green "BR" badge.
- Search by name with an on-screen keyboard.
- Favorites and play history, seeded from TWiLight Menu++'s history on the first run.
- Colors taken from the selected game's cover ("Cover art", the default), four fixed themes, or your own
  color themes as INI files. The details screen shows the cover over a blurred copy of itself.
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
| SELECT | Next filter |
| START | Options: filter, genre, sort order, view, grid art, theme, sounds, consoles, random game, rebuild library |
| Touch | Tap a game to select it, tap it again to play; tap a tab to open it; tap the filter or the sort order at the bottom to change it |
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

## With the SD card in the PC

One command backs up the saves, downloads box art for new games and updates genres and years:

```powershell
python tools\sync.py E:\
```

The sections below describe each step; `--no-backup`, `--no-covers` and `--no-metadata` skip one.

## Box art

To run only this step:

```powershell
python tools\fetch_covers.py E:\
```

It identifies each game (by game code for DS and GBA, by CRC32 for the other consoles), downloads its
cover (DS from GameTDB, the rest from libretro-thumbnails), scales it for the DSi and saves it to `sd:/_nds/DSCore/covers`. Only the Python
standard library is needed. Covers already present are skipped; `--force` downloads them again and
`--twilight` also saves the original images where TWiLight Menu++ looks for box art.

## Genres and years

```powershell
python tools\fetch_metadata.py E:\
```

It writes `sd:/_nds/DSCore/metadata.ini` with the genre, release year, developer and player count of
each game: DS games from GameTDB, the other consoles from libretro-database. Genres are grouped into a
short list (Action, Platform, RPG, Racing...). On the DSi, pick a genre under START > Genre, sort by
year, and see genre and year on the details screen. The file is plain text, one section per ROM file
name; add `locked = 1` to a section you edit by hand and new runs leave it alone. Other programs can
write the same format (see `arm9/source/core/Metadata.h`).

## Save backups

Saves live next to the games on the SD card, so a lost or damaged card loses them too. With the card
in the PC:

```powershell
python tools\backup_saves.py E:\
```

It copies every save under `roms` (including nds-bootstrap's `saves` folders), the emulator data under
`data` and DSCore's favorites and history to `DSCore saves\<date>` in your user folder (`--dest` picks
another place). Nothing is copied when nothing changed since the last backup. To put saves back:

```powershell
python tools\backup_saves.py --restore "C:\Users\you\DSCore saves\2026-09-29_214254" E:\
```

This only lists what would change; add `--apply` to copy. Saves it replaces are kept as
`<name>.before-restore`.

## Themes

Pick a theme in the options menu (START). "Cover art" tints both screens with the selected game's cover
(the color comes from `tools/fetch_covers.py`, so run it once after updating). To make your own, put an INI file in
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
| `sd:/_nds/DSCore/config.ini` | Last tab, filter, sort order, view, theme, hidden consoles and selected game |
| `sd:/_nds/DSCore/covers/` | Box art from `tools/fetch_covers.py` |
| `sd:/_nds/DSCore/metadata.ini` | Genres, years and developers from `tools/fetch_metadata.py` |
| `sd:/_nds/DSCore/thumbs.bin` | Grid thumbnails of the box art, rebuilt by every run of `tools/fetch_covers.py` |
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
