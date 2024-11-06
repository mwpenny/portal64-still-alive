#################
## Find Libgcc ##
#################

include(FindPackageHandleStandardArgs)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    execute_process(
        COMMAND
            ${CMAKE_C_COMPILER} -print-libgcc-file-name
        RESULT_VARIABLE
            GCC_COMMAND_RC
        OUTPUT_VARIABLE
            GCC_COMMAND_OUTPUT
        ERROR_VARIABLE
            GCC_COMMAND_ERROR
    )

    if (NOT GCC_COMMAND_RC EQUAL 0)
        message(SEND_ERROR "Error locating libgcc: ${GCC_COMMAND_ERROR}")
    else()
        cmake_path(GET GCC_COMMAND_OUTPUT
            PARENT_PATH Libgcc_LIBRARY_DIR
        )
        find_library(Libgcc_LIBRARY gcc
            HINTS ${Libgcc_LIBRARY_DIR}
        )
    endif()
endif()

find_package_handle_standard_args(Libgcc
    REQUIRED_VARS
        Libgcc_LIBRARY
)

if (Libgcc_FOUND AND NOT TARGET libgcc::libgcc)
    add_library(libgcc::libgcc STATIC IMPORTED)
    set_target_properties(libgcc::libgcc PROPERTIES
        IMPORTED_LOCATION ${Libgcc_LIBRARY}
    )
endif()
