###################
## Find libultra ##
###################

include(FindPackageHandleStandardArgs)

set(LIBULTRA_BOOT_CODE   "6102"             CACHE PATH   "Boot code to use")
set(LIBULTRA_RSP_UCODE   "gspF3DEX2.fifo"   CACHE PATH   "RSP microcode to use")

find_path   (Libultra_INCLUDE_DIR   ultra64.h           PATH_SUFFIXES "n64")
find_library(Libultra_LIBRARY       ultra_rom           PATH_SUFFIXES "n64")
find_package(Libgcc)

cmake_path(GET Libultra_LIBRARY
    PARENT_PATH Libultra_LIBRARY_DIR
)

find_file   (Libultra_BOOT          boot.${LIBULTRA_BOOT_CODE}  HINTS ${Libultra_LIBRARY_DIR}   PATH_SUFFIXES "PR/bootcode")
find_file   (Libultra_RSP_BOOT      rspboot.o                   HINTS ${Libultra_LIBRARY_DIR}   PATH_SUFFIXES "PR")
find_file   (Libultra_RSP_UCODE     ${LIBULTRA_RSP_UCODE}.o     HINTS ${Libultra_LIBRARY_DIR}   PATH_SUFFIXES "PR")
find_file   (Libultra_ASP_UCODE     aspMain.o                   HINTS ${Libultra_LIBRARY_DIR}   PATH_SUFFIXES "PR")

find_package_handle_standard_args(Libultra
    REQUIRED_VARS
        Libultra_INCLUDE_DIR
        Libultra_LIBRARY
        Libultra_RSP_BOOT
        Libultra_RSP_UCODE
        Libultra_ASP_UCODE
        Libgcc_LIBRARY
    HANDLE_COMPONENTS
)

if (Libultra_FOUND AND NOT TARGET libultra::libultra)
    add_library(libultra::libultra STATIC IMPORTED)
    set_target_properties(libultra::libultra PROPERTIES
        IMPORTED_LOCATION ${Libultra_LIBRARY}
    )
    target_include_directories(libultra::libultra INTERFACE
        "${Libultra_INCLUDE_DIR}"
        "${Libultra_INCLUDE_DIR}/PR"
    )
    target_compile_definitions(libultra::libultra INTERFACE
        F3DEX_GBI_2
        BOOT_CODE=${Libultra_BOOT}
    )
    target_link_libraries(libultra::libultra INTERFACE
        libgcc::libgcc
    )
endif()
