# Portal64

A demake of portal for the Nintendo 64

## How to build

First, you will need to setup [modern sdk](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html)

Next, you will need to download blender 2.9 or higher. Then set the environment variable `BLENDER_2_9` to be the absolute path where the blender executable is located on your system.

<br />

You will need to install python `vpk`
```
pip install vpk
```

<br />

Install `vtf2png`, `sfz2n64` and `skeletool64`
```sh
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" \
    | tee /etc/apt/sources.list.d/lambertjamesd.list

sudo apt install vtf2png sfz2n64 skeletool64 mpg123
```

<br />

Install image magic
```
sudo apt install imagemagick
```

<br />

You then need to add the following files from where portal is installed to the folder `vpk`
```
portal_pak_000.vpk  
portal_pak_001.vpk  
portal_pak_002.vpk  
portal_pak_003.vpk  
portal_pak_004.vpk  
portal_pak_005.vpk  
portal_pak_dir.vpk
```

Finally run `make` to build the project.

<br />


## Build with Docker


Build the docker image
```
docker build . -t portal64
```

<br />

Then build
```sh
# Set the environment variable
BLENDER_2_9=/blender/blender

# Build using docker
docker run \
    -v /home/james/Blender/blender-2.93.1-linux-x64:/blender \
    -e BLENDER_2_9 -v /home/james/portal/portal64/vpk:/usr/src/app/vpk \
    -t -v /home/james/portal/portal64/docker-output:/usr/src/app/build portal64
```

<br />

Where `/home/james/Blender/blender-2.93.1-linux-x64` is the folder where Blender is located.

`/home/james/portal/portal64/vpk` is the folder where the portal `*.vpk` files are located.

`/home/james/portal/portal64/docker-output` is where you want the output of the build to locate `portal.z64` will be put into this folder.

<br />

## Current TODO list

- [ ] Fix bug where opening a portal can trigger a teleportation
- [ ] Turn level indicator board into a game object
- [ ] Presort portal gun polygon order
- [ ] Implement level transitions
    - Implement loading levels from the cartridge
- [ ] Change the way player standing logic works
- [ ] Cube dispenser
- [ ] NAN in overlap
- [x] Implement "Elevator"
- [x] Implement "Emancipation grid"
- [x] Cut holes in portal walls
- [x] Get an optimized build working
- [x] Portal animations
- [x] Figure out why clip is silent
- [x] Fix z fighting in elevator
- [x] Fix crash
- [x] Determine why bad gfx cause RDP crash