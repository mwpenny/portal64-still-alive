###############
## Find Luac ##
###############

include(FindPackageHandleStandardArgs)

find_program(Luac_EXECUTABLE luac)

if(Luac_EXECUTABLE)
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
        message(SEND_ERROR "Error getting luac version: ${VERSION_COMMAND_ERROR}")
    elseif(VERSION_COMMAND_OUTPUT MATCHES "^Luac ([0-9\\.]+)")
        set(VERSION_NUMBER "${CMAKE_MATCH_1}")
    endif()
endif()

find_package_handle_standard_args(Luac
    REQUIRED_VARS
        Luac_EXECUTABLE
    VERSION_VAR
        VERSION_NUMBER
)
