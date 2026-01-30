#############################
## Set list cache variable ##
#############################

set(SUPPORTED_AUDIO_LANGUAGES
    english
    french
    german
    russian
    spanish
)

set(SUPPORTED_TEXT_LANGUAGES
    english
    brazilian
    bulgarian
    czech
    danish
    german
    spanish
    latam
    greek
    french
    italian
    polish
    hungarian
    dutch
    norwegian
    portuguese
    russian
    romanian
    finnish
    swedish
    turkish
    ukrainian
)

function(_comma_separated_to_list COMMA_LIST OUTPUT_VARIABLE)
    string(TOLOWER "${COMMA_LIST}" ${OUTPUT_VARIABLE})
    string(REPLACE "," ";" ${OUTPUT_VARIABLE} "${${OUTPUT_VARIABLE}}")
    return(PROPAGATE ${OUTPUT_VARIABLE})
endfunction()

# Define a list cache variable, which is validated against a set of choices.
# The string "all" can also be specified as a shortcut for all choices.
# The parsed list is returned in <VAR>_LIST.
function(list_option LIST_NAME DEFAULT_VALUE DESCRIPTION CHOICES_LIST)
    set(${LIST_NAME} ${DEFAULT_VALUE} CACHE STRING ${DESCRIPTION})
    _comma_separated_to_list("${${LIST_NAME}}" INPUT_LIST)

    # Need at least one entry
    if (NOT INPUT_LIST)
        string(REPLACE ";" ", " CHOICES "${CHOICES_LIST}")
        message(FATAL_ERROR
            "No values specified for list '${LIST_NAME}'. "
            "Valid entries: ${CHOICES} - or 'all'"
        )
    endif()

    # Expand "all" to all valid choices
    if ("all" IN_LIST INPUT_LIST)
        list(LENGTH INPUT_LIST INPUT_LENGTH)
        if (INPUT_LENGTH GREATER 1)
            message(FATAL_ERROR
                "Invalid entries in list '${LIST_NAME}'. "
                "Cannot specify 'all' with other entries."
            )
        endif()

        set(INPUT_LIST ${CHOICES_LIST})
    endif()

    # Handle individual entries
    foreach(ENTRY ${INPUT_LIST})
        if (NOT ${ENTRY} IN_LIST CHOICES_LIST)
            string(REPLACE ";" ", " CHOICES "${CHOICES_LIST}")
            message(FATAL_ERROR
                "Invalid entry '${ENTRY}' in list '${LIST_NAME}'. "
                "Valid entries: ${CHOICES} - or 'all'"
            )
        endif()
    endforeach()

    set(${LIST_NAME}_LIST ${INPUT_LIST})
    return(PROPAGATE ${LIST_NAME}_LIST)
endfunction()
