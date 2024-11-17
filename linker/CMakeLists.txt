###################
## Linker script ##
###################

set(LINKER_SCRIPT        "${CMAKE_CURRENT_SOURCE_DIR}/portal.ld")
set(OUTPUT_LINKER_SCRIPT "${LINKER_SCRIPT_DIR}/portal.ld")

# Add command to preprocess linker script
add_custom_command(
    DEPENDS
        ${LINKER_SCRIPT}
        anims_linker_script          "$<TARGET_PROPERTY:anims_linker_script,SCRIPT_FILE>"
        level_linker_script          "$<TARGET_PROPERTY:level_linker_script,SCRIPT_FILE>"
        models_dynamic_linker_script "$<TARGET_PROPERTY:models_dynamic_linker_script,SCRIPT_FILE>"
        string_linker_script         "$<TARGET_PROPERTY:string_linker_script,SCRIPT_FILE>"
    OUTPUT
        ${OUTPUT_LINKER_SCRIPT}
    COMMAND
        ${CMAKE_CPP} -P -Wall -Werror
        -I${LINKER_SCRIPT_DIR}
        -DRSP_BOOT=${Libultra_RSP_BOOT}
        -DRSP_UCODE=${Libultra_RSP_UCODE}
        -DASP_UCODE=${Libultra_ASP_UCODE}
        -o ${OUTPUT_LINKER_SCRIPT} ${LINKER_SCRIPT}
    COMMENT
        "Generating $<PATH:RELATIVE_PATH,${OUTPUT_LINKER_SCRIPT},${PROJECT_SOURCE_DIR}>"
    VERBATIM
)

add_custom_target(linker_script
    DEPENDS ${OUTPUT_LINKER_SCRIPT}
)
set_target_properties(linker_script PROPERTIES
    SCRIPT_FILE ${OUTPUT_LINKER_SCRIPT}
)
