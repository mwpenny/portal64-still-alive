OUTPUT_ARCH (mips)

#define BEGIN_SEG(name, addr) \
    _##name##SegmentStart = ADDR(.name); \
    _##name##SegmentRomStart = __romPos; \
    .name addr : AT(__romPos)

#define END_SEG(name) \
    _##name##SegmentEnd = ADDR(.name) + SIZEOF(.name); \
    _##name##SegmentRomEnd = __romPos + SIZEOF(.name); \
    __romPos += SIZEOF(.name);

#define BEGIN_NOLOAD(name) \
    _##name##SegmentBssStart = ADDR(.name.noload); \
    .name.noload (NOLOAD) :

#define END_NOLOAD(name) \
    _##name##SegmentBssEnd = ADDR(.name.noload) + SIZEOF(.name.noload); \
    _##name##SegmentBssSize = SIZEOF(.name.noload);


SECTIONS
{
   __romPos = 0;

   BEGIN_SEG(boot, 0x04000000)
   {
      build/asm/rom_header.o(.text);
      build/boot.o(.data);
   }
   END_SEG(boot)

   BEGIN_SEG(code, 0x80000400) SUBALIGN(16)
   {
      build/asm/entry.o(.text);
      CODE_SEGMENT(.text);
      build/rspboot.o(.text);
      build/gspMain.o(.text);
      build/aspMain.o(.text);

      /* data */
      CODE_SEGMENT(.data*);
      build/rspboot.o(.data*);
      build/gspMain.o(.data*);
      build/aspMain.o(.data*);

      /* rodata */
      CODE_SEGMENT(.rodata*);
   }
   END_SEG(code)
   BEGIN_NOLOAD(code)
   {
      CODE_SEGMENT(COMMON);
      CODE_SEGMENT(.scommon*);
      CODE_SEGMENT(.bss*);
      . = ALIGN(0x8);
   }
   END_NOLOAD(code)
   
   _codeSegmentBssEnd = .;
   
   _heapStart = .;

   . = 0x80200000;

   BEGIN_SEG(sound_data, __romPos)
   {
      build/asm/sound_data.o(.data);
      build/asm/sound_data.o(.bss);
   }
   END_SEG(sound_data)

   BEGIN_SEG(images, __romPos)
   {
      build/assets/materials/images_mat.o(.data);
      build/assets/materials/images_mat.o(.bss);
   }
   END_SEG(images)

#include "build/levels.ld"
#include "build/dynamic_models.ld"
#include "build/anims.ld"
#include "build/subtitles.ld"

   /* Discard everything not specifically mentioned above. */
   /DISCARD/ :
   {
      *(.eh_frame)
      *(.MIPS.abiflags)
   }
}