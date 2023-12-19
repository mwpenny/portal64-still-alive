## How to build

First, you will need to setup [Modern SDK](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html).

After installing modern sdk you will want to also install

```sh
sudo apt install libnustd
```

Next, you will need to install the latest version of Blender 3.6 LTS (please don't use e.g. 4.x, only 3.6.x will work correctly). Then set the environment variable `BLENDER_3_6` to be the absolute path where the Blender 3.6 executable is located on your system.

```sh
sudo snap install blender --channel=3.6lts/stable --classic
```

e.g. add this to your ~/.bashrc if you used snap (or you can use `which blender` to find the path of Blender 3.6)

```bash
export BLENDER_3_6="/snap/bin/blender"
```

<br />

You will need to install Python `vpk`:

```sh
sudo apt install pipx
pipx ensurepath
pipx install vpk
```

<br />

Clone the Portal64 repo or download the zip.

```sh
sudo apt install git
git clone https://github.com/lambertjamesd/portal64.git
cd portal64
```
Setup and install dependencies for `skelatool64`

```sh
echo "deb [trusted=yes] https://lambertjamesd.github.io/apt/ ./" \
    | sudo tee /etc/apt/sources.list.d/lambertjamesd.list
sudo apt update
sudo apt install vtf2png sfz2n64 mpg123 sox imagemagick unzip
``
```
Install ffmpeg 4.3.1
```
sudo snap install ffmpeg  # version 4.3.1

```

<br />

Install lua5.4 (You may need to remove other installed versions first, skelatool64 needs to be built with luac 5.4!)

```sh
sudo apt install lua5.4 liblua5.4-dev liblua5.4-0
```
<br />

You will need to install nodejs. You can use apt for this

```sh
sudo apt install nodejs
```
