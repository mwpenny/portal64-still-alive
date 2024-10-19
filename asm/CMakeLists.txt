####################
## Assembly files ##
####################

# TODO: proper dependencies (defs.h, macros.inc, sounds)

set(ASM_FILES
    entry.s
    rom_header.s
    sound_data.s
)

add_library(asm_sources OBJECT)
add_dependencies(asm_sources
    sound_tables
)
target_sources(asm_sources PRIVATE
    ${ASM_FILES}
)
