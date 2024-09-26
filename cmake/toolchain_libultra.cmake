include("${CMAKE_CURRENT_LIST_DIR}/toolchain_n64.cmake")
set(USING_LIBULTRA TRUE)

find_path(LIBULTRA_INCLUDE_DIR ultra64.h
    PATHS
        "${N64_TOOLCHAIN_PREFIX}/usr/include/n64"
    DOC
        "Libultra include directory"
    REQUIRED
)

set(LIBULTRA_INCLUDE_DIRS
    "${LIBULTRA_INCLUDE_DIR}"
    "${LIBULTRA_INCLUDE_DIR}/PR"
)

find_library(LIBULTRA_LIB libultra_rom
    PATHS
        "${N64_TOOLCHAIN_PREFIX}/usr/lib/n64"
    DOC
        "Libultra library file"
    REQUIRED
)

add_library(libultra STATIC IMPORTED)
set_target_properties(libultra PROPERTIES
    IMPORTED_LOCATION ${LIBULTRA_LIB}
)
target_include_directories(libultra
    INTERFACE ${LIBULTRA_INCLUDE_DIRS}
)
