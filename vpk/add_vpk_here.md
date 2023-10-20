# Source Portal Files

Copy the entire Portal folder, or create a symbolic link, in this folder

`ln -s SteamLibrary\steamapps\common\Portal\ Portal`

- If you want multi-language support, copy `portal_sound_vo_*.vpk` files commonly found in `SteamLibrary\steamapps\common\Portal\` to this directory
  - German:
  ```
  portal/portal_sound_vo_german_000.vpk
  portal/portal_sound_vo_german_dir.vpk
  ```

  - French:
  ```
  portal/portal_sound_vo_french_000.vpk
  portal/portal_sound_vo_french_dir.vpk
  ```

  - Russian:
  ```
  portal/portal_sound_vo_russian_000.vpk
  portal/portal_sound_vo_russian_dir.vpk
  ```

  - Spanish:
  ```
  portal/portal_sound_vo_spanish_000.vpk
  portal/portal_sound_vo_spanish_dir.vpk
  ```

The end results should be a folder structure like this

```
vpk/
  Portal/ -- symbolic link to Portal
  portal_sound_vo_german_000.vpk
  portal_sound_vo_german_dir.vpk
  portal_sound_vo_french_000.vpk
  portal_sound_vo_french_dir.vpk
  portal/portal_sound_vo_russian_000.vpk
  portal/portal_sound_vo_russian_dir.vpk
  portal/portal_sound_vo_spanish_000.vpk
  portal/portal_sound_vo_spanish_dir.vpk
```