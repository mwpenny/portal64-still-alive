###################
## N64 Toolchain ##
###################

set(CMAKE_SYSTEM_NAME Generic)

set(N64 TRUE)
set(N64_TOOLCHAIN_ROOT   ""            CACHE PATH   "Root directory of N64 toolchain")
set(N64_TOOLCHAIN_PREFIX "mips64-elf-" CACHE STRING "File name prefix for toolchain programs")

list(APPEND CMAKE_PREFIX_PATH
    "/usr"
)

# Search for programs in the host and target environment
# Search for headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH "${N64_TOOLCHAIN_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Don't try to dynamically link during compiler tests
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(CMAKE_ASM_COMPILER     ${N64_TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_C_COMPILER       ${N64_TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_CPP              ${N64_TOOLCHAIN_PREFIX}cpp REQUIRED)
find_program(CMAKE_CXX_COMPILER     ${N64_TOOLCHAIN_PREFIX}g++ REQUIRED)
find_program(CMAKE_LINKER           ${N64_TOOLCHAIN_PREFIX}ld  REQUIRED)

# General flags
set(COMPILE_FLAGS                   "-c -mabi=32 -mfix4300 -G 0 -Wall")
set(COMPILE_FLAGS_DEBUG             "-g")
set(COMPILE_FLAGS_DEBUGOPTIMIZED    "-g -Os")
set(COMPILE_FLAGS_RELEASE           "-DNDEBUG -Os")

# Program-specific flags
set(CMAKE_ASM_FLAGS_INIT            "${COMPILE_FLAGS} -x assembler-with-cpp -Wa,-I${PROJECT_SOURCE_DIR}")
set(CMAKE_C_FLAGS_INIT              "${COMPILE_FLAGS} -ffreestanding")
set(CMAKE_CXX_FLAGS_INIT            "${COMPILE_FLAGS} -fno-nonansi-builtins")
set(CMAKE_EXE_LINKER_FLAGS_INIT     "--no-check-sections")

# Propagate build type flags
set(BUILD_TYPES
    "Debug"
    "DebugOptimized"
    "Release"
)
foreach(LANG ASM C CXX)
    foreach(TYPE ${BUILD_TYPES})
        string(TOUPPER ${TYPE} TYPE_UPPER)
        set("CMAKE_${LANG}_FLAGS_${TYPE_UPPER}" ${COMPILE_FLAGS_${TYPE_UPPER}})
    endforeach()
endforeach()

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES ${BUILD_TYPES})
else()
    set(CMAKE_BUILD_TYPE "DebugOptimized" CACHE STRING "Project build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
endif()
