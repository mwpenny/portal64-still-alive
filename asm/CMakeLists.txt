####################
## Assembly files ##
####################

set(ASM_FILES
    entry.s
    rom_header.s
    sound_data.s
)

add_library(asm_sources INTERFACE)
add_dependencies(asm_sources
    sound_data_tables
)
target_sources(asm_sources INTERFACE
    ${ASM_FILES}
)
