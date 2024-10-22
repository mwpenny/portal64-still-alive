## How to Build

### SDK

First, you will need to setup [Modern SDK](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html).


### Game Dependencies

You can install most dependencies using your package manager's default repository:

```sh
sudo apt install build-essential cmake ffmpeg git imagemagick nodejs pip pipx python3 sox unzip
```

For the following dependencies, first add the following custom repository:

```sh
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" \
    | sudo tee /etc/apt/sources.list.d/lambertjamesd.list
sudo apt update
sudo apt install sfz2n64 vtf2png
```

Next, you will need to install Blender 3.6 LTS (please don't use e.g. 4.x, only 3.6.x will work correctly). Then set the environment variable `BLENDER_3_6` to be the absolute path where the Blender 3.6 executable is located on your system.

```sh
sudo snap install blender --channel=3.6lts/stable --classic
```

E.g., add this to your `~/.bashrc` if you used snap (or you can use `which blender` to find the path of Blender 3.6):

```bash
export BLENDER_3_6="/snap/bin/blender"
```

Install the Python `vpk` module using pipx:

```sh
sudo apt install pipx
pipx ensurepath
pipx install vpk
```

### Skeletool Dependencies

Setup and install dependencies for `skeletool64`:

```sh
sudo apt install cimg-dev libassimp-dev liblua5.4-0 liblua5.4-dev libpng-dev libtiff-dev libyaml-cpp-dev lua5.4
```

**Note:** Lua 5.4 is required!

### Getting the Code

Clone the Portal64 repo or download the zip.

```sh
git clone https://github.com/mwpenny/portal64-still-alive.git portal64
cd portal64
```
