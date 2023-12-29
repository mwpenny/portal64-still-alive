# Portal64
![](./assets/images/portal64_readme_logo.gif)

A demake *(remake for an older platform)* of Portal for the Nintendo 64.

![](./assets/images/readme_slideshow.gif)

Latest current progress video on Youtube:

[![Portal 64: First Slice, is out of beta](https://img.youtube.com/vi/sb3nHlsBBpg/0.jpg)](https://youtu.be/sb3nHlsBBpg)

## Download

You can download the ROM here if you own a copy of Portal for PC

[Rom Patcher](https://lambertjamesd.github.io/RomPatcher.js/index.html)

## Overview

This project aims to reproduce Valve's original Portal, playable on the N64. 

[Releases](https://github.com/lambertjamesd/portal64/releases) of this game are released in the form of a .bps patch, and produce a fully playable N64 ROM that can either be played through an emulator or on a physical N64 game cartridge. 

Please follow specific release instructions to get the ROM running on your target hardware.

Because this demake has been in development for over a year, it has made significant progress in both gameplay systems and fidelity including: 

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

This is a community driven project that welcomes any and all game testers and or [Contributors](./documentation/contributing.md). 

Updates are constantly being made to the game, so we recommend checking out the author's [YouTube Channel](https://www.youtube.com/@james.lambert) for the latest updates.

## How to build

Clone the Portal64 repo or download the zip.

```sh
sudo apt install git -y
git clone https://github.com/lambertjamesd/portal64.git
cd portal64
```

## Setup and install dependencies. 

The following commands allow the scripts to run on the system, then it runs the setup.

As always it is good practice look over scripts from the internet before running them on your system.

```sh
sudo chmod +x skelatool64/setup_dependencies.sh
sudo chmod +x tools/romfix64.sh
sudo chmod +x tools/setup.sh
./tools/setup.sh
```

Alternative setup methods include [Docker setup](./documentation/docker_setup.md) and [Manual setup](./documentation/manual_setup.md).

Whatever setup you choose, you will still need to add the Portal folder to portal64/`vpk/` OR create a symbolic link to the Portal folder.   

(see [vpk/add_vpk_here.md](./vpk/add_vpk_here.md) for more details!). Symlink does not work for Docker builds.


## Build Rom.

Finally, run `make` to build the project.

```sh
# Build (default build with english audio)
make
```

If you have issues use `make clean` to clean out any previous build files, remember it also removes any languages you set up so you will need to run those commands again.

```sh
# Clean out any previous build files
make clean
```
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
