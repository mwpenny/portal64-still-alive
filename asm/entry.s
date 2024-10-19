#include "../src/defs.h"

.include "macros.inc"

.section .text, "ax"

.glabel entry_point
    la      $t0, _codeSegmentNoLoadStart
    la      $t1, _codeSegmentNoLoadSize

.bss_clear:
    sw      $zero, ($t0)
    sw      $zero, 4($t0)
    addi    $t0, $t0, 8
    addi    $t1, $t1, -8
    bnez    $t1, .bss_clear

    la      $sp, mainStack + STACKSIZEBYTES
    j       boot
