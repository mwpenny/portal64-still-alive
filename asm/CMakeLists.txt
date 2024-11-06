####################
## Assembly files ##
####################

set(ASM_FILES
    boot.s
    entry.s
    rom_header.s
    sound_data.s
)

add_library(asm_sources OBJECT)
add_dependencies(asm_sources
    sound_data_tables
)
target_sources(asm_sources PRIVATE
    ${ASM_FILES}
)
target_link_libraries(asm_sources
    platform
)
