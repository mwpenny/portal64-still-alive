## How to build using System Packages and Steam

First, you will need to setup [Modern SDK](https://crashoveride95.github.io/n64hbrew/modernsdk/startoff.html).

Next, you will need to install the latest version of Blender 3.6 LTS from Steam using Steam Betas(4.x and above cause animation and model errors). Then set the environment variable `BLENDER_3_6` to be the absolute path where the Blender 3.6 executable is located on your system.

```
Steps:
1: Get blender from the Steam Store
2: Right-Click Blender in your library
3: Click Properties>Betas>none
4: Scroll and choose "v3.6 - Stable - LTS"
5: Close the window and click the Blue "INSTALL" button
```

e.g. add this to your ~/.bashrc (or you can use `which blender` to find the path of Blender 3.6)

```bash
export BLENDER_3_6="%PathToBlenderExecutable%"
```

This path can be in 1 of 3 places:
1: `~/.steam/debian-installation/steamapps/common/Blender/`
2: `/mnt/%DriveName%/SteamLibrary/steamapps/common/Blender`
3: `/media/%username%/%DriveID%/SteamLibrary/steamapps/common/Blender`

The install location is dependant on Drive/Mount type.

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
sudo add-apt-repository ppa:savoury1/ffmpeg4
sudo apt update
sudo apt install -y ffmpeg  # version 4.3.1

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
