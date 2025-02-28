# Sounds

All sounds are sourced from the original game's files. They are modified as
necessary before being used to generate sound banks which can be played back
at runtime.

## Sound Extraction and Conversion

Sounds must be extracted from the original game's files. They are stored inside
[VPK](https://developer.valvesoftware.com/wiki/VPK_(file_format)) archives and
extracted to `portal_pak_dir/`. See the page on
[original game files](./original_game_files.md) for details on VPK extraction.

Some sounds are localized. Depending on the audio language configuration at
build time, addional sounds will be extracted to `portal_pak_dir/localized/` for
inclusion in the game. As part of this process some may be renamed by the
[`tools/sound/normalize_sound_paths.js`](../../tools/sound/normalize_sound_paths.js)
script for consistency across languages. See
[vpk/README.md](../../vpk/README.md#add-multiple-audio-languages)
for more information on non-English audio.

Most sounds are stored in WAV format, which can be manipulated easily. Music is
stored as MP3 and so it is first converted to WAV using
[FFmpeg](https://www.ffmpeg.org/) and saved alongside the other extracted
sounds in `portal_pak_dir/`. The Valve intro audio is extracted from the intro
video file as WAV, also using FFmpeg, and output to `portal_pak_dir/` as well.

At this point, nothing has been modified - just converted.

## Sound Transformation

After all sounds are extracted and available as WAVs, most are modified to
better fit within N64 limitations. For example, reducing the number of channels,
downsampling, trimming, etc. Some sounds already fit the necessary criteria and
are not transformed.

Modification is done using [SoX](https://sourceforge.net/projects/sox/). The
arguments to pass are stored in files in `assets/sound/` and the resulting
sound files are output to `portal_pak_modified/` - each located at the same
relative path as the input sound in `portal_pak_dir/`.

For example:
* Input sound: `portal_pak_dir/sound/buttons/button3.wav`
* Arguments file: `assets/sound/buttons/button3.{sox,jsox}`.
* Transformed sound: `portal_pak_modified/sound/buttons/button3.wav`

Localized versions of the same sound are handled automatically and the output
is prefixed with `<language>_`.

There are two different ways to pass arguments to SoX, determined by arguments
file extension:

1. `.sox`: Text file containing arguments passed directly to SoX when generating
   the output WAV file. I.e., `sox INPUT ARGS OUTPUT`.
2. `.jsox`: JSON file containing `flags` and `filters` properties. Both are
   optional. The `flags` property is a string with the same contents as a `.sox`
   file. The `filters` property is a string containing effect options.
   I.e., `sox INPUT ARGS OUTPUT FILTERS`. Processing of these files is handled
   by [`tools/sound/jsox.js`](../../tools/sound/jsox.js).

At the end of this process `portal_pak_modified/sound/` will contain only
original-game sounds actually used by Portal 64.

## Sound Conversion

Before sound table data can be generated, sounds must be converted to AIFC.
These are compressed AIFF files using N64/libultra specific compression.
Conversion is done using [sfz2n64](https://github.com/lambertjamesd/sfz2n64).

Converted AIFC files are output to `<build directory>/assets/sound/` at the
same relative path as the input file.

## Sound Table Generation

After sounds are transformed and in the proper format then sound tables can be
generated.

### Sound Data

Sound data tables are generated using sfz2n64, which packs all audio data into
`<build directory>/assets/sound/sounds.sounds.tbl` and playback information
(enevelope, pan, loop, etc.) into `sounds.sounds`.

### Instrument Bank Configuration

Most sounds (those that have been transformed and converted to AIFC) are added
to the generated instrument banks as-is.

Some sounds require more control over parameters such as envelope, panning,
looping, etc. This additional configuration is specified in `.ins` files under
`assets/sounds/`, following the same directory structure as the SoX argument
files (same relative path as input file). It is possible to use this
configuration on both transformed and untransformed sounds. The functionality is
mainly used in Portal 64 for looping.

See https://github.com/lambertjamesd/sfz2n64 for more details.

### Lookup Tables

As part of sound processing, lookup tables are generated to allow the game to
refer to sounds at runtime, determine which audio languages are supported, and
map localized sounds to the player's selected language.

This information is output by
[`tools/sound/generate_sound_ids.js`](../../tools/sound/generate_sound_ids.js)
to generated C source files in `<build directory>/codegen/assets/audio/`.
