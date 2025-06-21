##############################
## Find ImageMagick convert ##
##############################

# The built-in find module for ImageMagick uses find_path, which will
# fail since the N64 toolchain sets CMAKE_FIND_ROOT_PATH_MODE_INCLUDE
# to "ONLY" (desirable). Use this simplified version instead.

include(FindPackageHandleStandardArgs)

find_program(ImageMagickConvert_EXECUTABLE convert)

if (ImageMagickConvert_EXECUTABLE)
    execute_process(
        COMMAND
            ${ImageMagickConvert_EXECUTABLE} -version
        RESULT_VARIABLE
            VERSION_COMMAND_RC
        OUTPUT_VARIABLE
            VERSION_COMMAND_OUTPUT
        ERROR_VARIABLE
            VERSION_COMMAND_ERROR
    )

    if (NOT VERSION_COMMAND_RC EQUAL 0)
        message(SEND_ERROR "Error getting ImageMagick version: ${VERSION_COMMAND_ERROR}")
    elseif (VERSION_COMMAND_OUTPUT MATCHES "^Version: ImageMagick ([-0-9\\.]+)")
        set(VERSION_NUMBER "${CMAKE_MATCH_1}")

        string(REPLACE "-" "." VERSION_NUMBER "${VERSION_NUMBER}")
    endif()
endif()

find_package_handle_standard_args(ImageMagickConvert
    REQUIRED_VARS
        ImageMagickConvert_EXECUTABLE
    VERSION_VAR
        VERSION_NUMBER
    HANDLE_VERSION_RANGE
)
