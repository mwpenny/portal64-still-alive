include("${CMAKE_CURRENT_LIST_DIR}/toolchain_n64.cmake")
set(LIBULTRA TRUE)

set(LIBULTRA_INCLUDE_DIRS
    "${N64_TOOLCHAIN_PREFIX}/usr/include/n64"
    "${N64_TOOLCHAIN_PREFIX}/usr/include/n64/PR"
)

# TODO: libs
