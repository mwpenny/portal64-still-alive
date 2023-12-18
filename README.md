# Portal64
![](./assets/images/portal64_readme_logo.gif)

A demake *(remake for an older platform)* of Portal for the Nintendo 64.

![](./assets/images/readme_slideshow.gif)

Latest current progress video on Youtube:

[![How I optimized Portal to run on the Nintendo 64](https://img.youtube.com/vi/r4KGm7ixcqI/0.jpg)](https://www.youtube.com/watch?v=r4KGm7ixcqI)

## Overview

This project aims to reproduce Valve's original Portal, playable on the N64. 

[Releases](https://github.com/lambertjamesd/portal64/releases) of this game are fully playable N64 ROM's that can either be played through an emulator or on a physical N64 game cartridge. 

Please follow specific release instructions to get ROM running on your target hardware.

Because this demake has been in development for many years, it has made significant progress in both gameplay systems and fidelity including: 

- Twelve+ test chambers completed
- Fully functioning portals, and gun
- Fully functioning physics engine
- Lighting system
- Main/pause menus
- Sound effects/dialogue
- Cutscenes
- Multi-language subtitles and audio dialogue
- Eye-Candy (Reflections, ...)
- Much more!

This is a community driven project that welcomes any and all game testers and or [Contributors](./CONTRIBUTING.md). 

Updates are constantly being made to the game, so we recommend checking out the author's [YouTube Channel](https://www.youtube.com/@james.lambert) for the latest updates.

## How to build

First, you will need to setup [Modern SDK](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html).

After installing modern sdk you will want to also install

```sh
sudo apt install libnustd
```

Next, you will need to install the latest version of Blender 3.6 LTS (please don't use e.g. 4.x, only 3.6.x will work correctly). Then set the environment variable `BLENDER_3_6` to be the absolute path where the Blender 3.6 executable is located on your system.

```sh
sudo snap install blender --channel=3.6lts/stable --classic
```

e.g. add this to your ~/.bashrc if you used snap (or you can use `which blender` to find the path of Blender 3.6)

```bash
export BLENDER_3_6="/snap/bin/blender"
```

<br />

You will need to install Python `vpk`:

```sh
sudo apt install pipx
pipx ensurepath
pipx install vpk
```

<br />

Clone the Portal64 repo or download the zip.

```sh
sudo apt install git
git clone https://github.com/lambertjamesd/portal64.git
cd portal64
```
Setup and install dependencies for `skelatool64`

```sh
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" \
    | sudo tee /etc/apt/sources.list.d/lambertjamesd.list
sudo apt update
sudo apt install vtf2png sfz2n64 mpg123 sox imagemagick unzip ffmpeg
```

<br />

Install lua5.4 (You may need to remove other installed versions first, skelatool64 needs to be built with luac 5.4!)

```sh
sudo apt install lua5.4 liblua5.4-dev liblua5.4-0
```

<br />

Setup and build skelatool64 (the version included in this portal64 repo!)

```sh
cd skelatool64
./setup_dependencies.sh
make
```

<br />

You will need to install nodejs. You can use apt for this

```sh
sudo apt install nodejs
```

<br />

You then need to add the following files from where Portal is installed to the folder `vpk/` OR create a symbolic link to the `Portal` folder there (see [vpk/add_vpk_here.md](./vpk/add_vpk_here.md) for more details!).
You can add multiple languages if desired.

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

Finally, run `make` to build the project.

```sh
# Clean out any previous build files
make clean

# Build (default build with english audio)
make
```
In case you have any trouble with the ROM running on hardware try padding the ROM.
```
make fix-nowine
```
You can also try the old way of paddinng the ROM, but you don't need both.

This method requires you to install wine to run properly. https://wiki.winehq.org/Ubuntu

```
make fix
```

Alternatively, you can also prepare to build with additional audio languages, like this (multiple commands per build possible):

```
make french_audio
make german_audio
make russian_audio
make spanish_audio
```
You still have run `make` after this.

Also you can build with all audio languages integrated with this shortcut:

```
make all_languages
```

This requires additional *.vpk files copied to the root of folder `vpk/` (since the original Portal keeps only one language at the same time, you have to copy these files!):

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

<br/>

## Build with Docker

Clone the Portal64 repo or download the zip.

```sh
sudo apt install git
git clone https://github.com/lambertjamesd/portal64.git
cd portal64
```

You need to populate the vpk folder - you need to COPY all needed *.vpk files, a symlink does not work for Docker builds!


After that you can build the docker image using


Build the Docker image.

```sh
make -f Makefile.docker image
```

<br />

Then build the rom using

```sh
make -f Makefile.docker
```

You can also use the language options, e.g.:

```sh
make -f Makefile.docker spanish_audio
make -f Makefile.docker german_audio
make -f Makefile.docker
```


That will generate the rom at `/build/portal64.z64`

<br />

## Current New Feature TODO List
- [ ] check if display list is long enough 
- [ ] pausing while glados is speaking can end her speech early
- [x] test chamber 10 without jumping
- [x] check collider flags when filtering contacts
- [x] gun flicker between levels
- [x] fizzler player sound effect
- [x] clear z buffer instead of partitioning it
- [X] add translations to menus
- [x] jump animation
- [x] optimize static culling
- [x] figure out why portals sometimes are in front of window
- [x] portal hole cutting problems
- [x] crashed when dying in test chamber 05 when hit by pellet in mid air while touching a portal
- [x] rumble pak support
- [x] valve intro
- [x] polish up subtitles
- [x] more sound settings
- [x] add desk chairs and monitors
- [x] Add auto save checkpoints
- [x] Correct elevator timing

## Current New Sounds TODO List
- [ ] Box collision sounds
- [x] Ambient background loop
- [x] Unstationary scaffolding moving sound

## Current Bug TODO List (Hardware Verified) (High->Low priority)
----------------------- v8
- [ ] Two wall portals next to eachother can be used to clip any object out of any level by pushing it into corner, then dropping. 
- [x] Passing into a ceiling portal can sometimes mess with the player rotation
- [x] player can clip through back of elevator by jumping and strafeing at the back corners while inside.
