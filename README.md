# Portal64

A demake of portal for the Nintendo 64

## How to build

First, you will need to setup [modern sdk](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html)

Next, you will need to download blender 2.9 or higher. Then set the environment variable `BLENDER_2_9` to be the absolute path where the blender executable is located on your computer.

You will need to install python vpk

```
pip install vpk
```

Install vtf2png and skeletool64

```
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" | tee /etc/apt/sources.list.d/lambertjamesd.list
sudo apt install vtf2png skeletool64
```

Install image magic

```
sudo apt install imagemagick
```

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

Finally run `make` to build the project

