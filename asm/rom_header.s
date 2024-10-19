/* PI_BSD_DOM1 configuration */
.byte   0x80                    /* Big endian indicator                               */
.byte   0x37                    /* Upper nybble: release, lower nybble: page size     */
.byte   0x12                    /* Pulse width                                        */
.byte   0x40                    /* Latency                                            */

.word   0x0000000F              /* Clock rate (masked to 0; default)                  */
.word   entry_point             /* Entry point                                        */
.word   0x0000144C              /* Libultra version (2.0L)                            */
.skip   8                       /* Checksum (OVERWRITTEN BY MAKEMASK)                 */
.skip   8                       /* Unused                                             */

.ascii  "Portal 64           "  /* Game title                                         */
.word   0x01000000              /* Unused / Homebrew header: 1 controller with rumble */
.skip   3                       /* Unused                                             */
.ascii  "N"                     /* Game format (cartridge)                            */
.ascii  "ED"                    /* Game ID / Homebrew header: magic value             */
.ascii  "A"                     /* Region (all)                                       */
.byte   0x32                    /* Version / Homebrew header: region-free + 256K SRAM */
