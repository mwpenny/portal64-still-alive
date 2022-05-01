# Portal64

A demake of portal for the Nintendo 64

## How to build

First, you will need to setup [modern sdk](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html)

Next, you will need to downlaod blender 2.9 or higher. Then set the environment variable `BLENDER_2_9` to be the absolute path where the blender executable is located on your computer.

You will need to instal python vpk

```
pip install vpk
```

Build and install [vtf2png](https://github.com/eXeC64/vtf2png)

Install image magic

```
sudo apt install imagemagic
```

Finally run `make` to build the project