OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips", "elf32-littlemips")
OUTPUT_ARCH("mips")

#define BEGIN_SEG(name, addr) \
    _##name##SegmentStart = ADDR(.name); \
    _##name##SegmentRomStart = __romPos; \
    .name addr : AT(__romPos)

#define END_SEG(name) \
    _##name##SegmentEnd = ADDR(.name) + SIZEOF(.name); \
    _##name##SegmentRomEnd = __romPos + SIZEOF(.name); \
    __romPos += SIZEOF(.name);

#define BEGIN_NOLOAD(name) \
    _##name##SegmentNoLoadStart = ADDR(.name.noload); \
    .name.noload (NOLOAD) :

#define END_NOLOAD(name) \
    _##name##SegmentNoLoadEnd = ADDR(.name.noload) + SIZEOF(.name.noload); \
    _##name##SegmentNoLoadSize = SIZEOF(.name.noload);


SECTIONS
{
   __romPos = 0;

   BEGIN_SEG(boot, 0x04000000)
   {
      */rom_header.*(.text);
      */boot.*(.text);
   }
   END_SEG(boot)

   BEGIN_SEG(code, 0x80000400) SUBALIGN(16)
   {
      /* code */
      */entry.*(.text);
      CODE_SEGMENT(.text);
      RSP_OBJ(.text);
      GSP_OBJ(.text);
      ASP_OBJ(.text);

      /* data */
      CODE_SEGMENT(.data*);
      RSP_OBJ(.data*);
      GSP_OBJ(.data*);
      ASP_OBJ(.data*);

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
   
   _codeSegmentNoLoadEnd = .;
   
   _heapStart = .;

   . = 0x80200000;

   BEGIN_SEG(sound_data, __romPos)
   {
      */sound_data.*(.data);
      */sound_data.*(.bss);
   }
   END_SEG(sound_data)

   BEGIN_SEG(images, __romPos)
   {
      *assets/materials/images_mat.*(.data);
      *assets/materials/images_mat.*(.bss);
   }
   END_SEG(images)

#include "levels.ld"
#include "dynamic_models.ld"
#include "anims.ld"
#include "subtitles.ld"

   /* Discard standard sections not mentioned above, keep debug info */
   /DISCARD/ :
   {
      *(.text)
      *(.data*)
      *(.rodata*)
      *(.bss*)
      *(.eh_frame)
      *(.MIPS.abiflags)
   }
}
