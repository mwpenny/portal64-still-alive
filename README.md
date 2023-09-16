# Portal64
![](./assets/images/portal64_readme_logo.gif)

A demake of Portal for the Nintendo 64.

## Overview

This project aims to reproduce valve's original portal, playable on the N64. [Releases](https://github.com/lambertjamesd/portal64/releases) of this game are fully playable N64 ROM's that can either be played through an emulator or on a physical N64 game cartridge. Please follow specific release instructions to get ROM running on your target hardware.  Because this demake has been in development for many years, it has made significant progress in both gameplay systems and fidelity including: 
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
```

Finally, run `make` to build the project.
```sh
# Clean out any previous build files
make clean

# Build
make

# In case you have any trouble with ROM running on hardware try
# wine install required to run properly
sudo apt install wine
make fix
```

<br />


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
- [ ] Change default controls
- [ ] investigate failed portal hole in test chamber 04
- [ ] increaes collider height
- [ ] ball velocity in test chamber 11
- [ ] clear sleeping object physics flag so buttons in savefiles remain pressed after loading
- [ ] rumble pak support?
- [ ] Investigate crash after falling into death water on test chamber 8
- [ ] Add particle effects (shooting portal gun, energy pellet)
- [ ] Add auto save checkpoints
- [ ] Correct elevator timing
- [ ] Adding loading notice between levels #45
- [ ] pausing while glados is speaking can end her speech early
- [x] test chamber 11 is broken
- [x] test chamber 4 has an inverted indicator light sign 
- [x] test chamber 4 door doesnt close
- [x] Use a much nearer clipping plane when rendering the portal gun
- [x] test chamber 04 has seams in a corner
- [x] Add puzzle element connections and additional signs
- [x] don't count boxes on buttons until it is released and stable
- [x] Portal not rendering recursively sometimes #138
- [x] disable portal surfaces manually on some surfaces #135
- [x] test chamber 02 needs more light in the first room
- [x] Presort portal gun polygon order #102

## Current New Sounds TODO List
- [ ] Box collision sounds
- [ ] Unstationary scaffolding moving sound
- [ ] Ambient background loop

## Current Bug TODO List (Hardware Verified) (High->Low priority)
----------------------- v8
- [ ] player can clip through back of elevator by jumping and strafeing at the back corners while inside.
- [ ] Two wall portals next to eachother can be used to clip any object out of any level by pushing it into corner, then dropping. 
- [ ] Passing into a ceiling portal can sometimes mess with the player rotation
- [ ] various visual glitches when running NTSC on PAL console #65
- [ ] various visual glitches when running PAL on NTSC console #65
- [x] Player can trap themselves in chamber 5 by following instructions issue #75
- [x] Can shoot portals, and walk through signage
- [x] Can place portals on ground after final fizzler on all levels
