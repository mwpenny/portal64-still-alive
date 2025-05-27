#define _STR(s) #s
#define STR(s) _STR(s)

.section .text, "ax"

.incbin     STR(BOOT_CODE)
