# Native Build Setup

These steps will install and configure the project dependencies system-wide.
You can also follow the same steps to set up the dependencies in a virtual
machine or using Windows Subsystem for Linux (WSL). Follow them manually, or
execute the [tools/setup_ubuntu.sh](../../tools/setup_ubuntu.sh) script.

> **Note:** The steps and commands below assume you are using Ubuntu Linux or
a similar Debian derivative (we test the same version as the
[Docker image](../../Dockerfile#L1)). Building in other environments is possible
but considered advanced -- you should know what you're doing if trying something
different. Otherwise, consider [using Docker](./docker_setup.md).

## SDK

First, you will need to set up
[Modern SDK](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html).
Follow the instructions on the linked page.

## Game Dependencies

You can install most dependencies using the default Ubuntu repositories.
If using something other than `apt` or Ubuntu then package names and
availability may differ.

```sh
sudo apt install build-essential cmake ffmpeg git imagemagick nodejs pip pipx python3 sox unzip
```

For the following dependencies, first add the following custom repository.
You will need to build [sfz2n64](https://github.com/lambertjamesd/sfz2n64) and
[vtf2png](https://github.com/eXeC64/vtf2png) from source if not using Ubuntu.

```sh
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" \
    | sudo tee /etc/apt/sources.list.d/lambertjamesd.list
sudo apt update
sudo apt install sfz2n64 vtf2png
```

Next, you will need to install Blender 3.6 LTS (the version is important; only
3.6.x will work correctly). Snap is an easy way to install a specific version.
You could alternatively download it from
[blender.org](https://download.blender.org/release/Blender3.6/) or other means,
just ensure the Blender executable directory is included in the `PATH`
environment variable so it can be found at build time.

```sh
sudo snap install blender --channel=3.6lts/stable --classic
```

Install the Python `vpk` module using `pipx`.

```sh
sudo apt install pipx
pipx ensurepath
pipx install vpk
```

## Skeletool Dependencies

Install dependencies for `skeletool64`. This tool is used to convert some game
assets.

> **Note:** Lua must be version 5.4!

```sh
sudo apt install cimg-dev libassimp-dev liblua5.4-0 liblua5.4-dev libpng-dev libtiff-dev libyaml-cpp-dev lua5.4
```

## Building the Code

From here, follow the [build instructions](./building.md).
