# Fonts

The game includes both a monospaced (Liberation Mono) and variable-width (DejaVu
Sans) font. Original font files are not included in this repository. Rather,
bitmap versions of each font are stored in `assets/fonts/<font name>/` along
with glyph metadata.

## Font Creation

The bitmap versions of each font are created from the original font using
[fontbuilder](https://github.com/andryblack/fontbuilder).

For each font, the used characters are output in 128x64 images ("pages"), which
are suitable for use as 4-bit intensity textures. The characters to use are
listed in the `font_characters.txt` file in each font directory (one page per
line).

The fontbuilder tool generates JSON files during export of each image, which
contain information about each glyph (e.g., spacing, unicode code point,
kerning, etc.). If a font has multiple pages, a JSON file for all characters
also needs to be generated for the kerning information since characters from
different pages can appear next to each other in practice.

This step has already been done, and is how the files in each
`assets/fonts/<font name>/` directory were created. It only needs to be done
when updating fonts.

## Font Generation

At build time, the
[`tools/text/font_converter.js`](../../tools/text/font_converter.js) script
generates C code for each font. This code contains glyph information (including
texture ID, texture coordinates, and kerning information) which is used in font
rendering.

Generated code is output to `<build directory>/codegen/assets/fonts/`.
