##################
## Find Node.js ##
##################

include(FindPackageHandleStandardArgs)

find_program(NodeJs_EXECUTABLE node)

if (NodeJs_EXECUTABLE)
    execute_process(
        COMMAND
            ${NodeJs_EXECUTABLE} --version
        RESULT_VARIABLE
            VERSION_COMMAND_RC
        OUTPUT_VARIABLE
            VERSION_COMMAND_OUTPUT
        ERROR_VARIABLE
            VERSION_COMMAND_ERROR
    )

    if (NOT VERSION_COMMAND_RC EQUAL 0)
        message(FATAL_ERROR "Error getting Node.js version: ${VERSION_COMMAND_ERROR}")
    elseif (NOT VERSION_COMMAND_OUTPUT MATCHES "^v([0-9\\.]+)")
        message(FATAL_ERROR "Unexpected output getting Node.js version:\n${VERSION_COMMAND_OUTPUT}")
    else()
        set(VERSION_NUMBER "${CMAKE_MATCH_1}")
    endif()
endif()

find_package_handle_standard_args(NodeJs
    REQUIRED_VARS
        NodeJs_EXECUTABLE
    VERSION_VAR
        VERSION_NUMBER
)
