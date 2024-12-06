# Source Portal Files

Game assets from Portal are sourced from the original game's files, which must be supplied separately. Copy the entire Portal folder to this directory
(`vpk/`).

If you are not using Docker, you can use a symbolic link.

```sh
ln -s <SteamLibrary>/steamapps/common/Portal vpk/Portal
```

At a minimum, the following directory structure is required. However, it is
easier and less error prone to just copy everything.

```
vpk/
└── Portal/
    ├── hl2/
    │   ├── media/
    │   │   └── valve.bik
    │   ├── resource/
    │   ├── hl2_misc_000.vpk
    │   ├── hl2_misc_001.vpk
    │   ├── hl2_misc_002.vpk
    │   ├── hl2_misc_003.vpk
    │   ├── hl2_misc_dir.vpk
    │   ├── hl2_sound_misc_000.vpk
    │   ├── hl2_sound_misc_001.vpk
    │   ├── hl2_sound_misc_002.vpk
    │   └── hl2_sound_misc_dir.vpk
    └── portal/
        ├── resource/
        ├── portal_pak_000.vpk
        ├── portal_pak_001.vpk
        ├── portal_pak_002.vpk
        ├── portal_pak_003.vpk
        ├── portal_pak_004.vpk
        ├── portal_pak_005.vpk
        └── portal_pak_dir.vpk
```

## Add Multiple Audio Languages

The original game supports English, French, German, Russian, and Spanish audio.
This project only uses English audio by default. If you want audio languages
other than English, follow the steps below for each desired additional language.

1. Change the current language for Portal in Steam to the desired language using
   the game properties menu. The game will update.
2. Open `<SteamLibrary>/steamapps/common/Portal/portal` and find the
   `portal_sound_vo_*.vpk` files corresponding to the new language.
3. Copy the language audio VPKs to `vpk/`. **These files must be copied because
   Steam only keeps the files of one language at a time.**

When building, set the `AUDIO_LANGUAGES` variable appropriately. See
[Building the Game](../documentation/building/building.md#optional-settings).
