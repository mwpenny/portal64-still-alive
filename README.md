# Portal64
![](./assets/images/portal64_readme_logo.gif)

A demake of Portal for the Nintendo 64.

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


Build the Docker image.
```sh
make -f Makefile.docker build
```

<br />

Then build.
```sh
make -f Makefile.docker
```

<br />

## Current New Feature TODO List
- [ ] Add auto save checkpoints
- [ ] rotate auto uv
- [ ] disable portal surfaces manually on some surfaces #135
- [ ] Portal not rendering recursively sometimes #138
- [ ] Correct elevator timing
- [ ] Presort portal gun polygon order #102
- [ ] Adding loading notice between levels #45
- [ ] Vertex lighting #39
- [x] Adding y-axis/x-axis inverting options #55
- [x] Adding look axis sensitivity setting
- [x] Adding a menu to game #47
- [x] Multi controller support #23
- [x] force placing auto portals when there is a conflict
- [x] Camera shake
- [x] Portal gun movement with player movement/shooting #19

## Current New Sounds TODO List
- [ ] Box collision sounds
- [ ] Unstationary scaffolding moving sound
- [ ] Ambient background loop
- [x] Button release beep-beep sound
- [x] Elevator arrived chime sound
- [x] Ball catcher activated sound
- [x] Fast flying air whoosh sound

## Current Bug TODO List (Hardware Verified) (High->Low priority)
- [ ] investigate chell animation
----------------------- v8
- [ ] Player can clip through chamber 7 by walking back up the stairs (near the top).
- [ ] player can clip through back of elevator by jumping and strafeing at the back corners while inside.
- [ ] Player can trap themselves in chamber 5 by following instructions issue #75
- [ ] Two wall portals next to eachother can be used to clip any object out of any level by pushing it into corner, then dropping. 
- [ ] Glass can be walked through from one side on multiple levels (0,1,4,...)
- [ ] Passing into a ceiling portal can sometimes mess with the player rotation
- [ ] Can shoot portals, and walk through signage
- [ ] Chell animation problem (fixed itself, investigate)
- [ ] Can place portals on ground after final fizzler on all levels
- [ ] Door at end of room 2, chamber 10 isnt rendered properly
- [ ] various visual glitches when running NTSC on PAL console #65
- [ ] various visual glitches when running PAL on NTSC console #65
- [x] Any grabbable object can be clipped through level by wall/floor portals method.
- [x] Player can clip through any level by placing one portal on wall and another portal right next to it on ground. #13
- [x] Can shoot portals while holding an object
