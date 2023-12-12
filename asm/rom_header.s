/*
 * ROM header
 * Only the first 0x18 bytes matter to the console.
 */

.byte  0x80, 0x37, 0x12, 0x40   /* PI BSD Domain 1 register */
.word  0x0000000F               /* Clockrate setting*/
.word  entry_point              /* Entrypoint */

/* SDK Revision */
.word  0x0000144C

.word  0x00000000               /* Checksum 1 (OVERWRITTEN BY MAKEMASK)*/
.word  0x00000000               /* Checksum 2 (OVERWRITTEN BY MAKEMASK)*/
.word  0x00000000               /* Unknown */
.word  0x00000000               /* Unknown */
.ascii "Portal 64           "   /* Internal ROM name (Max 20 characters) */

.word  0x01000000               /* Unused officially / Advanced homebrew ROM header controller config */
.word  0x0000004E               /* Cartridge Type (N; cart)*/
.ascii "ED"                     /* Cartridge ID (ED) / Advanced homebrew ROM header magic value */
.byte  0x41                     /* Region (A; All)*/
.byte  0x32                     /* Version / Advanced homebrew ROM header misc. (region-free + 256K SRAM) */
