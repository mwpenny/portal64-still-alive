# Portal64: Still Alive

A demake *(remake for an older platform)* of Portal for the Nintendo 64.
Originally created by [James Lambert](https://github.com/lambertjamesd).

The original repository was taken down by James at Valve's request, due to the build requirement on Nintendo's proprietary libultra and its consequent inclusion in ROM files.

The [original](https://github.com/mwpenny/portal64/tree/original) branch contains the version history of the original repository. Active development occurs in the [master](https://github.com/mwpenny/portal64/tree/master) branch.

The main goals of this fork are:
1. Remove proprietary code requirement (see [N64 Libraries](./documentation/n64_library_usage.md))
2. Finish development of the game (see [Development Progress](./documentation/development_progress.md))

We do what we must because we can.

![](./assets/images/readme_slideshow.gif)

## Disclaimer

This project is not affiliated with Nintendo or Valve.

This repository contains no material owned by Nintendo. However, Nintendo's tools are currently required to build the game. **No form of compiled ROM will be distributed while this dependency is required** since the built game would contain Nintendo's intellectual property. One goal of this project is to remove the requirement on proprietary code.

Game assets from Portal are sourced from the original game's files, which must be supplied separately at build time. In other words, **this repository cannot be used to compile the game without legally owning Portal and providing its files**. Legal ownership of Portal will be required regardless of tool or library changes.

## Overview

This project aims to reproduce Valve's original Portal, playable on the N64.

Because this demake has been in development for some time, it has made significant progress in both gameplay systems and fidelity including:

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

## How to build

Clone the Portal64 repo or download the zip.

```sh
sudo apt install git -y
git clone https://github.com/mwpenny/portal64-still-alive.git portal64
cd portal64
```

### Setup and install dependencies

The following commands allow the scripts to run on the system, then it runs the setup.

As always it is good practice look over scripts from the internet before running them on your system.

```sh
sudo chmod +x skelatool64/setup_dependencies.sh
sudo chmod +x tools/romfix64.sh
sudo chmod +x tools/setup.sh
./tools/setup.sh
```

Alternative setup methods include [Docker setup](./documentation/docker_setup.md) and [Manual setup](./documentation/manual_setup.md).

Whatever setup you choose, you will still need to add the Portal folder to `portal64/vpk/` OR create a symbolic link to the Portal folder. See [vpk/add_vpk_here.md](./vpk/add_vpk_here.md) for more details! Symlinks do not work for Docker builds.

### Build ROM

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
