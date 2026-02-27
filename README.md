# Portal64: Still Alive

A demake *(remake for an older platform)* of Portal for the Nintendo 64.
Originally created by [James Lambert](https://github.com/lambertjamesd).

The original repository was taken down by James at Valve's request, due to the
build requirement on Nintendo's proprietary libultra and its consequent
inclusion in ROM files.

The [original](https://github.com/mwpenny/portal64/tree/original) branch
contains the version history of the original repository. Active development
occurs in the [master](https://github.com/mwpenny/portal64/tree/master) branch.

The main goals of this fork are:
1. Remove proprietary code requirement (see [N64 Libraries](./documentation/n64_library_usage.md))
2. Finish development of the game (see [Development Progress](./documentation/development_progress.md))

We do what we must because we can.

![](./assets/images/readme_slideshow.gif)

## Disclaimer

This project is not affiliated with Nintendo or Valve.

This repository contains no material owned by Nintendo. However, Nintendo's
tools are currently required to build the game. **No form of compiled ROM will
be distributed while this dependency is required** since the built game would
contain Nintendo's intellectual property. One goal of this project is to remove
the requirement on proprietary code.

Game assets from Portal are sourced from the original game's files, which must
be supplied separately at build time. In other words, **this repository cannot
be used to compile the game without legally owning Portal and providing its
files**. Legal ownership of Portal will be required regardless of tool or
library changes. You can buy the game [here](https://store.steampowered.com/app/400/Portal/).

## Overview

This project aims to reproduce Valve's original Portal, playable on the N64.

Because this demake has been in development for some time, it has made
significant progress in both gameplay systems and fidelity including:

- Seventeen test chambers completed
- Fully functioning portals, and gun
- Fully functioning physics engine
- Lighting system
- Main/pause menus
- Sound effects/dialogue
- Cutscenes
- Multi-language subtitles and audio dialogue
- Eye candy (reflections, ...)
- Much more!

This is a community driven project that welcomes any and all game testers and or
[contributors](./documentation/contributing.md).

## How to Build

### Get the Code

First, clone the repository or download a source code archive.

```sh
git clone https://github.com/mwpenny/portal64-still-alive.git portal64
```

### Provide Game Files

You must provide your own legally obtained Portal game files so the game assets
can be extracted and used. Follow the instructions at
[vpk/README.md](./vpk/README.md).

### Install Dependencies

Next, install the dependencies. There are two supported ways to do this:

* [Use Docker](./documentation/building/docker_setup.md). This is recommended if
  you don't want to make system-wide changes or are unfamiliar with Linux.
* [Install natively](./documentation/building/native_setup.md). This is also
  applicable if using a virtual machine or Windows Subsystem for Linux (WSL).

### Build the Game

With everything set up, follow the instructions at
[Building the Game](./documentation/building/building.md).

To run and save properly, the game expects a cartridge with 32 KB of SRAM.
