# Source Portal Files

Copy the entire Portal folder, or create a symbolic link, in this folder

```
ln -s SteamLibrary\steamapps\common\Portal\ Portal
```

- If you want multi-language support, copy `portal_sound_vo_*.vpk` files commonly found in `SteamLibrary\steamapps\common\Portal\` to this directory.
  (these files need to be copied, because the original Portal keeps only the files of ONE language at a time)
  Change the current language for Portal in Steam to the desired language after Portal updates to the new language, copy the new language files and paste them in portal64/vpk.    

  Do this for each one of the languages you want to add to the ROM.

  - German:
  ```
  portal/portal_sound_vo_german_000.vpk
  portal/portal_sound_vo_german_dir.vpk
  ```
  
  ```
  make german_audio         # Set up the files to add German audio to the ROM.
  ```
  
  - French:
  ```
  portal/portal_sound_vo_french_000.vpk
  portal/portal_sound_vo_french_dir.vpk
  ```
  
  ```
  make french_audio         # Set up the files to add French audio to the ROM.
  ```

  - Russian:
  ```
  portal/portal_sound_vo_russian_000.vpk
  portal/portal_sound_vo_russian_dir.vpk
  ```
  
  ```
  make russian_audio         # Set up the files to add Russian audio to the ROM.
  ```

  - Spanish:
  ```
  portal/portal_sound_vo_spanish_000.vpk
  portal/portal_sound_vo_spanish_dir.vpk
  ```
  ```
  make spanish_audio         # Set up the files to add Spanish audio to the ROM.       
  ```
  

The end results should be a folder structure like this

```
vpk/
  Portal/ -- symbolic link to Portal
  portal_sound_vo_german_000.vpk
  portal_sound_vo_german_dir.vpk
  portal_sound_vo_french_000.vpk
  portal_sound_vo_french_dir.vpk
  portal_sound_vo_russian_000.vpk
  portal_sound_vo_russian_dir.vpk
  portal_sound_vo_spanish_000.vpk
  portal_sound_vo_spanish_dir.vpk
```

```
make all_languages         # This command runs `make` for you after the language files are set up.   
```
## Add multipal subtile languages.

If desired place the following files in portal64/vpk.
```
Portal/portal/portal_pak_000.vpk  
Portal/portal/portal_pak_001.vpk  
Portal/portal/portal_pak_002.vpk  
Portal/portal/portal_pak_003.vpk  
Portal/portal/portal_pak_004.vpk
Portal/portal/portal_pak_005.vpk  
Portal/portal/portal_pak_dir.vpk

Portal/hl2/hl2_sound_misc_000.vpk
Portal/hl2/hl2_sound_misc_001.vpk
Portal/hl2/hl2_sound_misc_002.vpk
Portal/hl2/hl2_sound_misc_dir.vpk

Portal/hl2/hl2_misc_000.vpk
Portal/hl2/hl2_misc_001.vpk
Portal/hl2/hl2_misc_002.vpk
Portal/hl2/hl2_misc_003.vpk
Portal/hl2/hl2_misc_dir.vpk

Portal/hl2/media/valve.bik

Portal/hl2/resource/gameui_english.txt
Portal/hl2/resource/gameui_<your desired language 1>.txt
Portal/hl2/resource/gameui_<your desired language 2>.txt

Portal/hl2/resource/valve_english.txt
Portal/hl2/resource/valve_<your desired language 1>.txt
Portal/hl2/resource/valve_<your desired language 2>.txt

Portal/portal/resource/closecaption_english.txt
Portal/portal/resource/closecaption_<your desired language 1>.txt
Portal/portal/resource/closecaption_<your desired language 2>.txt

Portal/portal/resource/portal_english.txt
Portal/portal/resource/portal_<your desired language 1>.txt
Portal/portal/resource/portal_<your desired language 2>.txt

```