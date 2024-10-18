##################
## Find Blender ##
##################

include(FindPackageHandleStandardArgs)

find_program(Blender_EXECUTABLE blender)

if(Blender_EXECUTABLE)
    execute_process(
        COMMAND
            ${Blender_EXECUTABLE} --version
        RESULT_VARIABLE
            VERSION_COMMAND_RC
        OUTPUT_VARIABLE
            VERSION_COMMAND_OUTPUT
        ERROR_VARIABLE
            VERSION_COMMAND_ERROR
    )

    if (NOT VERSION_COMMAND_RC EQUAL 0)
        message(SEND_ERROR "Error getting Blender version: ${VERSION_COMMAND_ERROR}")
    elseif(VERSION_COMMAND_OUTPUT MATCHES "^Blender ([0-9\\.]+)")
        set(VERSION_NUMBER "${CMAKE_MATCH_1}")
    endif()
endif()

find_package_handle_standard_args(Blender
    REQUIRED_VARS
        Blender_EXECUTABLE
    VERSION_VAR
        VERSION_NUMBER
)
