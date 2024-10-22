#####################################
## Add external project executable ##
#####################################

include(ExternalProject)
include(GNUInstallDirs)

# Adds a local external project and creates an imported executable target
# Assumes the project's directory and executable have the same name
function(add_external_project_executable PROJECT_NAME)
    set(RELATIVE_EXE_PATH "${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}")

    ExternalProject_Add(${PROJECT_NAME}
        SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}"
        PREFIX
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}"
        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
        BUILD_BYPRODUCTS
            # Needed for Ninja so we can depend on the executable before it exists
            "<INSTALL_DIR>/${RELATIVE_EXE_PATH}"
        BUILD_ALWAYS
            # Make sure it gets rebuilt when changes are made
            TRUE
    )

    ExternalProject_Get_Property(${PROJECT_NAME} INSTALL_DIR)

    set(EXE_TARGET_NAME ${PROJECT_NAME}::${PROJECT_NAME})
    add_executable(${EXE_TARGET_NAME} IMPORTED)
    add_dependencies(${EXE_TARGET_NAME} ${PROJECT_NAME})
    set_target_properties(${EXE_TARGET_NAME} PROPERTIES
        IMPORTED_LOCATION "${INSTALL_DIR}/${RELATIVE_EXE_PATH}"
    )

endfunction()
