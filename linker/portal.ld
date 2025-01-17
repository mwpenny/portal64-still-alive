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
      *code_segment.*:(.text);
      *libultra_rom.*:(.text);
      *libgcc.*:(.text);
      RSP_BOOT(.text);
      RSP_UCODE(.text);
      ASP_UCODE(.text);

      /* data */
      *code_segment.*:(.data*);
      *libultra_rom.*:(.data*);
      *libgcc.*:(.data*);
      RSP_BOOT(.data*);
      RSP_UCODE(.data*);
      ASP_UCODE(.data*);

      /* rodata */
      *code_segment.*:(.rodata*);
      *libultra_rom.*:(.rodata*);
      *libgcc.*:(.rodata*);
   }
   END_SEG(code)

   BEGIN_NOLOAD(code)
   {
      *code_segment.*:(.bss*);
      *libgcc.*:(.bss*);

      *libultra_rom.*:(COMMON);
      *libultra_rom.*:(.scommon*);
      *libultra_rom.*:(.bss*);

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
      */assets/materials/images_mat.*(.data);
      */assets/materials/images_mat.*(.bss);
   }
   END_SEG(images)

#include "levels.ld"
#include "dynamic_models.ld"
#include "anims.ld"
#include "strings.ld"

   .padding :
   {
      BYTE(0);
      . = ALIGN(__romPos + 1, 512) - __romPos;
   }

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
