###############
## Find CImg ##
###############

include(FindPackageHandleStandardArgs)

find_path(CImg_INCLUDE_DIR CImg.h)

find_package_handle_standard_args(CImg
    REQUIRED_VARS
        CImg_INCLUDE_DIR
)

if(CImg_FOUND AND NOT TARGET cimg::cimg)
    add_library(cimg::cimg INTERFACE IMPORTED)
    target_include_directories(cimg::cimg INTERFACE
        ${CImg_INCLUDE_DIR}
    )
endif()
