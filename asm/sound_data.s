.include "macros.inc"

.section .data

glabel _soundsSegmentRomStart
.incbin "build/assets/sound/sounds.sounds"
.balign 16
glabel _soundsSegmentRomEnd

glabel _soundsTblSegmentRomStart
.incbin "build/assets/sound/sounds.sounds.tbl"
.balign 16
glabel _soundsTblSegmentRomEnd