# Source Portal Files

Copy the entire Portal folder, or create a symbolic link, in this folder (`vpk/`).

```sh
ln -s SteamLibrary\steamapps\common\Portal\ Portal
```

At a minimum, the following directory structure is required:

```
vpk/
└── Portal/
    ├── hl2/
    │   ├── media/
    │   │   └── valve.bik
    │   |── resource/
    │   |   ├── gameui_english.txt
    │   |   └── valve_english.txt
    |   ├── hl2_misc_000.vpk
    |   ├── hl2_misc_001.vpk
    |   ├── hl2_misc_002.vpk
    |   ├── hl2_misc_003.vpk
    |   ├── hl2_misc_dir.vpk
    |   ├── hl2_sound_misc_000.vpk
    |   ├── hl2_sound_misc_001.vpk
    |   ├── hl2_sound_misc_002.vpk
    |   └── hl2_sound_misc_dir.vpk
    └── portal/
        ├── resource/
        |   ├── closecaption_english.txt
        |   └── portal_english.txt
        ├── portal_pak_000.vpk
        ├── portal_pak_001.vpk
        ├── portal_pak_002.vpk
        ├── portal_pak_003.vpk
        ├── portal_pak_004.vpk
        ├── portal_pak_005.vpk
        └── portal_pak_dir.vpk
```

## Add multiple audio languages

Only English is audio included by default. If you want multi-language support,
copy the `portal_sound_vo_*.vpk` files found in `SteamLibrary/steamapps/common/Portal/portal`
to `vpk/`. These files need to be copied, because the original Portal keeps only
the files of ONE language at a time.

Change the current language for Portal in Steam to the desired language. After
Portal updates to the new language, copy the new language files and paste them
in `vpk/`.

Do this for each one of the languages you want to add to the ROM.

* German:
  ```
  portal/portal_sound_vo_german_000.vpk -> vpk/
  portal/portal_sound_vo_german_dir.vpk -> vpk/
  ```
  ```sh
  make german_audio
  ```
  
* French:
  ```
  portal/portal_sound_vo_french_000.vpk -> vpk/
  portal/portal_sound_vo_french_dir.vpk -> vpk/
  ```
  ```
  make french_audio
  ```

* Russian:
  ```
  portal/portal_sound_vo_russian_000.vpk -> vpk/
  portal/portal_sound_vo_russian_dir.vpk -> vpk/
  ```
  ```
  make russian_audio
  ```

* Spanish:
  ```
  portal/portal_sound_vo_spanish_000.vpk -> vpk/
  portal/portal_sound_vo_spanish_dir.vpk -> vpk/
  ```
  ```
  make spanish_audio
  ```

* All languages:
  ```
  portal_sound_vo_german_000.vpk  -> vpk/
  portal_sound_vo_german_dir.vpk  -> vpk/
  portal_sound_vo_french_000.vpk  -> vpk/
  portal_sound_vo_french_dir.vpk  -> vpk/
  portal_sound_vo_russian_000.vpk -> vpk/
  portal_sound_vo_russian_dir.vpk -> vpk/
  portal_sound_vo_spanish_000.vpk -> vpk/
  portal_sound_vo_spanish_dir.vpk -> vpk/
  ```
  ```sh
  # This will also build the ROM
  make all_languages
  ```

After setting up your desired languages, you can run `make` to build the ROM as
normal.

## Add multiple subtile languages

To include additional subtitle languages, copy the following files from the
original Portal game directory into the corresponding locations under `vpk/Portal/`
(see above).

```
hl2/resource/gameui_<language>.txt
hl2/resource/valve_<language>.txt
portal/resource/closecaption_<language>.txt
portal/resource/portal_<language>.txt
```

If copying or symlinking the entire Portal game directory to `vpk/`, these are
picked up automatically.
