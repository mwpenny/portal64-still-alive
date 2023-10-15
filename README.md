# Portal64
![](./assets/images/portal64_readme_logo.gif)

A demake of Portal for the Nintendo 64.

## Overview

This project aims to reproduce Valve's original Portal, playable on the N64. [Releases](https://github.com/lambertjamesd/portal64/releases) of this game are fully playable N64 ROM's that can either be played through an emulator or on a physical N64 game cartridge. Please follow specific release instructions to get ROM running on your target hardware.  Because this demake has been in development for many years, it has made significant progress in both gameplay systems and fidelity including: 
- Twelve+ test chambers completed
- Fully functioning portals, and gun
- Fully functioning physics engine
- Lighting system
- Main/pause menus
- Sound effects/dialogue
- Cutscenes
- Much more!

This is a community driven project that welcomes any and all game testers and or [Contributors](./CONTRIBUTING.md). Updates are constantly being made to the game, so we recommend checking out the author's [YouTube Channel](https://www.youtube.com/@happycoder1989) for the latest updates.

![](./assets/images/readme_slideshow.gif)


## How to build

First, you will need to setup [Modern SDK](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html).

After installing modern sdk you will want to also install

```sh
sudo apt install libnustd
```

Next, you will need to download Blender 3.0 or higher. Then set the environment variable `BLENDER_3_0` to be the absolute path where the Blender executable is located on your system.

```sh
sudo apt install blender
```

e.g. add this to your ~/.bashrc

```bash
export BLENDER_3_0="/usr/bin/blender"
```

<br />

You will need to install Python `vpk`.
```sh
pip install vpk
```

<br />

Install `vtf2png`, `sfz2n64`, and setup `skeletool64`.
```sh
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" \
    | sudo tee /etc/apt/sources.list.d/lambertjamesd.list
sudo apt update
sudo apt install vtf2png sfz2n64 mpg123 sox imagemagick unzip
```

<br />

Install lua5.4 (remove other perhaps installed versions first, skelatool64 needs to be build with luac 5.4!)

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

You will need to add at least one of the following files from where Portal is installed to the folder `resource/`. You can add multiple languages if desired.
```
portal/resource/closecaption_english.txt
portal/resource/closecaption_<your desired language 1>.txt
portal/resource/closecaption_<your desired language 2>.txt
```

You then need to add the following files from where Portal is installed to the folder `vpk`. (see vpk/add_vpk_here.md  for more details!)
```
portal/portal_pak_000.vpk  
portal/portal_pak_001.vpk  
portal/portal_pak_002.vpk  
portal/portal_pak_003.vpk  
portal/portal_pak_004.vpk  
portal/portal_pak_005.vpk  
portal/portal_pak_dir.vpk

hl2/hl2_sound_misc_000.vpk
hl2/hl2_sound_misc_001.vpk
hl2/hl2_sound_misc_002.vpk
hl2/hl2_sound_misc_dir.vpk

hl/hl2_misc_000.vpk
hl/hl2_misc_001.vpk
hl/hl2_misc_002.vpk
hl/hl2_misc_003.vpk
hl/hl2_misc_dir.vpk
```

Finally, run `make` to build the project.
```sh
# Clean out any previous build files
make clean

# Build (default english audio build)
make

# In case you have any trouble with ROM running on hardware try
# wine install required to run properly
sudo apt install wine
make fix
```

Alternatively, you can also build with different audio languages, like this:
```
make german_audio
make french_audio
make russian_audio
make spanish_audio
```

This requires additional *.vpk files:

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

Using the docker image the only setup step you need is to populating the vpk folder. After that you can build the docker image using

Build the Docker image.
```sh
make -f Makefile.docker image
```

<br />

Then build the rom using
```sh
make -f Makefile.docker
```

That will generate the rom at `/build/portal64.z64`

<br />

## Current New Feature TODO List
- [ ] polish up subtitles
- [ ] more sound settings
- [ ] rumble pak support
- [ ] pausing while glados is speaking can end her speech early
- [x] add desk chairs and monitors
- [x] Add auto save checkpoints
- [x] Correct elevator timing

## Current New Sounds TODO List
- [ ] Box collision sounds
- [ ] Ambient background loop
- [x] Unstationary scaffolding moving sound

## Current Bug TODO List (Hardware Verified) (High->Low priority)
----------------------- v8
- [ ] Two wall portals next to eachother can be used to clip any object out of any level by pushing it into corner, then dropping. 
- [ ] Passing into a ceiling portal can sometimes mess with the player rotation
- [ ] various visual glitches when running NTSC on PAL console #65
- [ ] various visual glitches when running PAL on NTSC console #65
- [x] player can clip through back of elevator by jumping and strafeing at the back corners while inside.
