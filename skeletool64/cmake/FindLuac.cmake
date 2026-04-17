###############
## Find Luac ##
###############

include(FindPackageHandleStandardArgs)

set(PROGRAM_NAMES luac)
if (Luac_FIND_VERSION_COUNT GREATER 1)
    list(PREPEND PROGRAM_NAMES luac${Luac_FIND_VERSION_MAJOR}.${Luac_FIND_VERSION_MINOR})
endif()

find_program(Luac_EXECUTABLE NAMES ${PROGRAM_NAMES})

if (Luac_EXECUTABLE)
    execute_process(
        COMMAND
            ${Luac_EXECUTABLE} -v
        RESULT_VARIABLE
            VERSION_COMMAND_RC
        OUTPUT_VARIABLE
            VERSION_COMMAND_OUTPUT
        ERROR_VARIABLE
            VERSION_COMMAND_ERROR
    )

    if (NOT VERSION_COMMAND_RC EQUAL 0)
        message(FATAL_ERROR "Error getting luac version: ${VERSION_COMMAND_ERROR}")
    elseif (NOT VERSION_COMMAND_OUTPUT MATCHES "^Lua ([0-9\\.]+)")
        message(FATAL_ERROR "Unexpected output getting luac version:\n${VERSION_COMMAND_OUTPUT}")
    else()
        set(VERSION_NUMBER "${CMAKE_MATCH_1}")
    endif()
endif()

find_package_handle_standard_args(Luac
    REQUIRED_VARS
        Luac_EXECUTABLE
    VERSION_VAR
        VERSION_NUMBER
)
