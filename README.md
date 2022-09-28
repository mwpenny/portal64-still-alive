# Portal64

A demake of portal for the Nintendo 64

## How to build

First, you will need to setup [modern sdk](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html)

Next, you will need to download blender 3.0 or higher. Then set the environment variable `BLENDER_3_0` to be the absolute path where the blender executable is located on your system.

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
BLENDER_3_0=/blender/blender

# Build using docker
docker run \
    -v /home/james/Blender/blender-2.93.1-linux-x64:/blender \
    -e BLENDER_3_0 -v /home/james/portal/portal64/vpk:/usr/src/app/vpk \
    -t -v /home/james/portal/portal64/docker-output:/usr/src/app/build portal64
```

<br />

Where `/home/james/Blender/blender-2.93.1-linux-x64` is the folder where Blender is located.

`/home/james/portal/portal64/vpk` is the folder where the portal `*.vpk` files are located.

`/home/james/portal/portal64/docker-output` is where you want the output of the build to locate `portal.z64` will be put into this folder.

<br />

## Current TODO list

- [ ] stop looping sounds betwen levels
- [ ] calculateBarycentricCoords when two points are the same
- [ ] Z buffer allocation
- [ ] Release grabbed objects when line of sight is cut
- [ ] Correct elevator timing
- [ ] Elevator and door sounds
- [ ] Presort portal gun polygon order
- [ ] Cube dispenser
- [ ] Signage should not always be on
- [ ] Camera shake
- [x] level transition jump
- [x] collide player with dynamic objects
- [x] Render objects intersecting portals differently
- [x] Sliding against walls
- [x] It is too easy to fall through portals
- [x] Change the way player standing logic works
- [x] crash on level transition
- [x] Prevent Glados from talking over herself
- [x] NAN in overlap
- [x] Turn level indicator board into a game object
- [x] kill plane
- [x] Portal gun pedistal
- [x] Fix portal overlapping bug
- [x] Fix bug where opening a portal can trigger a teleportation
- [x] Implement level transitions