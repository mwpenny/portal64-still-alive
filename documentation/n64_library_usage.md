# N64 Library Usage

Portal 64 was originally written using Nintendo's proprietary libraries and tools. This material is not included in this repository, but ROMs built with it will necessarily contain Nintendo code. This is the reason Portal64: Still Alive currently does not distribute compiled ROMs in any form.

One major goal of this fork is to remove the requirement on proprietary code. The most mature open-source option is [libdragon](https://github.com/DragonMinded/libdragon). When Portal 64 originally began development, libdragon did not support 3D graphics. Since then, a full OpenGL 1.1 port has been implemented in its [unstable](https://github.com/DragonMinded/libdragon/tree/unstable) branch, along with open-source boot code (IPL3) and many other features!

## Current Non-Free Library Usage

Portal 64's high-level usage of proprietary libraries is listed in the following table:

| Library  | Subsystem                     | Usage                                        | Free equivalent implemented? |
| -------- | ----------------------------- | -------------------------------------------- | ---------------------------- |
| libultra | Audio (al)                    | Music                                        | ❌                          |
|          |                               | Sound effects                                | ❌                          |
|          | Display processing (gDP/gsDP) | Graphics rendering<sup>*</sup>               | ❌                          |
|          | Signal processing (gSP/gsSP)  | 3D Math, lighting, culling, etc.<sup>*</sup> | ❌                          |
|          | Graphics common (gdSP)        | Lighting                                     | ❌                          |
|          | Graphics utility (gu)         | 3D math                                      | ❌                          |
|          | Operating system (os)         | Address translation                          | ❌                          |
|          |                               | DMA                                          | ❌                          |
|          |                               | Timers                                       | ❌                          |
|          |                               | Interrupts                                   | ❌                          |
|          |                               | Task scheduling                              | ❌                          |
|          |                               | Message passing                              | ❌                          |
|          |                               | Peripheral access                            | ❌                          |
|          |                               | Video initialization                         | ❌                          |
|          | Math                          | Trig functions                               | ❌                          |
| libnustd | Math                          | `floor()`                                    | ✅                          |
|          | String                        | `strcpy()`, `memset()`                       | ✅                          |

<sup>*</sup> = Not only the game uses these APIs. At build time, `Skeletool64`
reads YAML files located in `assets/materials/` to generate code containing
static display lists. The YAML files, generated code, and Skeletool itself all
refer to xDP/xSP function names and parameters.

## Migration Strategy

Using new libraries in such core areas is a large task, but as long as no
binaries containing proprietary code are distributed, changes can be done
incrementally so the effort is manageable. The table in the previous section
will track this progress (and a dedicated document will be created if necessary).

Additionally, since nothing containing Nintendo libraries will be distributed
while they are still a requirement, regular progress on the game itself can also
continue at the same time. There is no need to drop everything and rewrite large
portions of the codebase all at once before continuing -- rather, technical debt
can gradually be addressed alongside regular development so that the pace of the
project remains steady.

**Important:** Nintendo's libraries will not be fully removed at first. Instead,
a library-independent layer will be built to allow compiling with _either_
libultra or libdragon. This has several benefits:

1. Testability of changes (can compare with original implementation)
2. Emulator compatibility
3. Cleaner code
4. Easier porting to other platforms in the future
