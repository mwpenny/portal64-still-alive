# Original Game Files

Portal 64 uses assets from the original game. It sources most of these from its
[VPK](https://developer.valvesoftware.com/wiki/VPK_(file_format)) files, and
the rest from text files (for strings) along with a `.bik`/`.mov` file for the
Valve intro video. All files must be legally obtained by the user and provided
at build time.

The build process looks for the original game's files in the `vpk/` directory.
See [vpk/README.md](../../vpk/README.md) for information on the required
directory structure.

## VPK Extraction

VPK files are archives and so they must be extracted to make use of the files
within. The archives are extracted into the `portal_pak_dir/` directory using
the [vpk](https://pypi.org/project/vpk/) Python module.

## Asset Transformation and Conversion

Some Portal files use formats which are not easy to work with (for example,
[VTF](https://developer.valvesoftware.com/wiki/VTF) textures). Where relevant, assets are first converted to common formats such as PNG and WAV
before any further manipulation takes place. These converted-but-unmodified
files are saved in `portal_pak_dir/` alongside the originals.

Parts of the pipeline that modify original game assets (e.g., image cropping,
sound resampling, etc.) output the resulting files to the `portal_pak_modified/`
directory. Files in this directory still use standard formats that are easy to
open. This provides a good preview of what the in-game result will be. They are
not yet in their final N64-ready form, but no more modifications will be done.

Finally, when either modified or unmodified assets are converted for inclusion
in the game, those files are output to the build directory. Generated C source
code goes to `<build directory>/codegen/assets/`. Binary assets go to
`<build directory>/assets/`.
