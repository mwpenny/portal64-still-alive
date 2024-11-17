# Docker Build Setup

This project provides a Docker image for easy and consistent builds regardless
of platform. This is the recommended setup if you don't want to make system-wide
changes just for this project, or are unfamiliar with Linux.

## Install Docker

Install Docker either by using your package manager or by following the
instructions at https://docs.docker.com/engine/install/.

For example, on Ubuntu:

```sh
sudo apt install docker.io
```

If installing from docker.com, Docker Desktop is not required -- only Docker
Engine.

## Build Docker Image

Build the Docker image with the following command. This will create an image
tagged with the name "portal64" containing all project dependencies.

```sh
cd portal64
docker build -t portal64 .
```

## File Permissions

If you run Docker as root you may want to change the permissions of the
`portal_pak_dir/`, `portal_pak_modified/`, and build output directories to be
able to edit them outside of the container (they will be owned by root). On
Linux, you can use the following commands.

```sh
# Replace <build_directory> with build directory name
sudo chmod 777 -R portal_pak_dir
sudo chmod 777 -R portal_pak_modified
sudo chmod 777 -R <build_directory>
```

## Running Commands in the Container

Once the Docker image is built, use the following command to launch a container
and access an interactive shell.

```sh
cd portal64
docker run --rm -v .:/usr/src/app -it portal64 bash
```

This mounts the current directory (the project root) inside the container at
`/usr/src/app` (so the build can read the project and game files) and runs
`bash` to provide a shell.

## Building the Code

From here, all build commands are the same as a native build. Follow the
[build instructions](./building.md) as normal making sure to run the commands
in the container. You can exit by using the `exit` command, after which the
container will be removed.
