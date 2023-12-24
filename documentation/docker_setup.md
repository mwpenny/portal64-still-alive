## Build with Docker

If `apt install docker` doesn't work, you have to either use `apt install docker.io` or e.g. not use apt at all and install Docker "officially" from docker.com      


Also, using snap to install Docker is not advised `sudo snap install docker` gives, an [Errno 13] Permission denied error message (for python3.11-minimal) and this blocks creating the Docker image.

(also, some may have to run `sudo setfacl -R -m u:$USER:rwx /var/run/docker.sock` first, if Docker wasn't installed from docker.com)


Add all desired [languages](/vpk/add_vpk_here.md) for the build process.        

After that you can build the docker image using:


```sh

make f Makefile.docker image    # Builds the Docker image.

```

Then build the rom using;

```sh

make -f Makefile.docker

```

## You can also use the language options, e.g.:

```sh

make -f Makefile.docker german_audio

```

```sh

make -f Makefile.docker french_audio

```

```sh

make -f Makefile.docker russian_audio

```

```sh

make -f Makefile.docker spanish_audio

```

```sh

make -f Makefile.docker  # Run this after running the commands for the desired languages that you would like to add to your ROM.

```

## Build all audio languages into the ROM.

Make sure to put all the following files in the portal64/vpk folder then run:
  
```sh

make -f Makefile.docker all_languages

```
If you have issues use `make -f Makefile.docker clean` to clean out any previous build files, remember it also removes any languages you set up so you will need to run those commands again.

```sh

make -f Makefile.docker clean

```

That will generate the rom at `/build/portal64.z64`       

If you run Docker in sudo you may want to change the permissions of portal64/build, portal64/portal_pak_dir and portal64portal_pak_modified:
```sh
sudo chmod 777 -R build
sudo chmod 777 -R portal_pak_dir
sudo chmod 777 -R portal_pak_modified

```
