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
.ascii "                    "   /* Internal ROM name (Max 20 characters) */
.word  0x01000000               /* Advanced_Homebrew_ROM_Header (controller config) */
/* Game ID (EXAMPLE: NSME) Begins here */
.word  0x0000004E                /* Cartridge Type (N)*/
.ascii "ED"                     /* Cartridge ID (SM)*/
.ascii " "			/* Region (E)*/
.byte  0x30			/* Version */
