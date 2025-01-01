# Building the Game

> **Note:** Make sure you have
> [copied your Portal game files](../../vpk/README.md) and
> set up the project dependencies either
> [using Docker](./docker_setup.md) or [natively](./native_setup.md).

This project is built using [CMake](https://cmake.org/cmake/help/). CMake works
in two main phases: **configuration** and **building**.

CMake supports multiple different target build systems. Configuration time is
when build system files are generated. For example, Makefiles or `.ninja` files.
Configuration only needs to happen when first setting up the project or when
changing build-related settings.

Build time is when the code is actually compiled/linked. This amounts to running
the build tool corresponding to the files CMake previously generated at
configuration time (`make`, `ninja`, etc.).

## CMake Generators

In CMake, a generator is what creates the files for a particular target build
system. Ninja is optional but recommended due to its speed. You may need to
install it. For example, on Debian/Ubuntu:

```sh
apt install ninja-build
```

## Configuration

Configure the build system with the following command. Portal 64 is
cross-compiled for the N64, and so a toolchain file must be specified.

The default CMake generator is system dependent. We specify an exact generator
with the `-G` argument for consistency across platforms. You can use "Unix
Makefiles" in the configuration command instead of "Ninja" to use `make`.

```sh
cd portal64

# Replace <build_directory> with build directory name
cmake -G "Ninja" -B <build_directory> -S . -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-N64.cmake
```

### Manually Specifying Missing Dependencies

CMake will try to automatically locate dependencies on your system. If programs
or directories cannot be found during configuration then you can explicitly
assign values to relevant variables (mentioned in error messages) using the `-D`
argument. For example:

```sh
cd portal64

# Replace <build_directory> with build directory name
cmake -DBlender_EXECUTABLE=/path/to/blender <build_directory>
```

This should not be necessary if you followed the dependency setup steps, but is
useful to know in case of issues or if using a different build environment than
these documents.

## Build

Once the build system has been configured, build the game with one of the
following commands. From this point forward only build commands are needed.

```sh
cd portal64

# Generator independent
# Replace <build_directory> with build directory name
cmake --build <build_directory>
```

or

```sh
# Replace <build_directory> with build directory name
cd portal64/<build_directory>

# If using ninja
ninja

# If using make
make
```

## Clean

You can use the following commands to remove generated files and get back to a
clean state.

```sh
cd portal64

# Generator independent
# Replace <build_directory> with build directory name
cmake --build <build_directory> --target clean
```

or

```sh
# Replace <build_directory> with build directory name
cd portal64/<build_directory>

# If using ninja
ninja clean

# If using make
make clean
```

## Optional Settings

There are several settings you can change which affect the build. Most notably:

| Name              | Type                 | Description |
| ----------------- | -------------------- | --- |
| `AUDIO_LANGUAGES` | Comma-separated list | Specify which audio languages to include. Supported values are any combination of `english`, `french`, `german`, `russian`, or `spanish` - or just `all`, to include everything. Ensure relevant audio VPKs have been copied (see [vpk/README.md](../../vpk/README.md#add-multiple-audio-languages)). Only English audio is included by default. |
| `TEXT_LANGUAGES`  | Comma-separated list | Specify which text languages to include. Supported values are any combination of `english`, `brazilian`, `bulgarian`, `czech`, `danish`, `german`, `spanish`, `latam`, `greek`, `french`, `italian`, `polish`, `hungarian`, `dutch`, `norwegian`, `portuguese`, `russian`, `romanian`, `finnish`, `swedish`, `turkish`, or `ukrainian` - or just `all`, to include everything. All supported text languages are included by default. |
| `DEBUGGER`        | Boolean              | Build with support for hardware debugging. See [debugger.md](../debugger.md) for more information. Defaults to `OFF`. |
| `GFX_VALIDATOR`   | Boolean              | Build with display list validator. Defaults to `OFF`. |
| `RSP_PROFILER`    | Boolean              | Build with RSP performance profiler. Defaults to `OFF`. |

You can see a list of all project CMake variables using the following commands.

```sh
# Standard variables
cmake -LH build

# Standard and advanced variables
cmake -LAH build
```

Update a variable using the following command, then build again.

```sh
cd portal64

# Replace <build_directory> with build directory name
cmake -DVARIABLE_NAME=value <build_directory>
```
