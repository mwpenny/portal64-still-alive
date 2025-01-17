###################
## N64 Toolchain ##
###################

set(CMAKE_SYSTEM_NAME Generic)
set(N64 TRUE)

set(N64_TOOLCHAIN_ROOT   ""          CACHE PATH   "Root directory of N64 toolchain")
set(N64_TOOLCHAIN_PREFIX "mips-n64-" CACHE STRING "File name prefix for toolchain programs")

# Ensure CMake can find toolchain during compiler tests
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    N64_TOOLCHAIN_ROOT
    N64_TOOLCHAIN_PREFIX
)

list(APPEND CMAKE_PREFIX_PATH
    "/"
    "/usr"
)

# Programs may be in the host or target environment
# We should never use headers or libraries from the host
set(CMAKE_FIND_ROOT_PATH "${N64_TOOLCHAIN_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

find_program(CMAKE_ASM_COMPILER     ${N64_TOOLCHAIN_PREFIX}gcc      REQUIRED)
find_program(CMAKE_C_COMPILER       ${N64_TOOLCHAIN_PREFIX}gcc      REQUIRED)
find_program(CMAKE_CPP              ${N64_TOOLCHAIN_PREFIX}cpp      REQUIRED)
find_program(CMAKE_CXX_COMPILER     ${N64_TOOLCHAIN_PREFIX}g++      REQUIRED)
find_program(CMAKE_OBJCOPY          ${N64_TOOLCHAIN_PREFIX}objcopy  REQUIRED)
find_program(Makemask_EXECUTABLE    makemask                        REQUIRED)  # TODO: support different boot code

# General flags
set(COMPILE_FLAGS                   "-mabi=32 -mfix4300 -G 0 -Wall -Werror")
set(COMPILE_FLAGS_DEBUG             "-g")
set(COMPILE_FLAGS_DEBUGOPTIMIZED    "-g -Os")
set(COMPILE_FLAGS_RELEASE           "-g -DNDEBUG -Os")

# Program-specific flags
set(CMAKE_ASM_FLAGS_INIT            "${COMPILE_FLAGS} -x assembler-with-cpp -Wa,-I\"${PROJECT_BINARY_DIR}\"")
set(CMAKE_C_FLAGS_INIT              "${COMPILE_FLAGS} -ffreestanding")
set(CMAKE_CXX_FLAGS_INIT            "${COMPILE_FLAGS} -fno-nonansi-builtins")
set(CMAKE_EXE_LINKER_FLAGS_INIT     "-nostdlib -Wl,--no-check-sections")

# Propagate build type flags
set(BUILD_TYPES
    "Debug"
    "DebugOptimized"
    "Release"
)
foreach(LANG ASM C CXX)
    foreach(TYPE ${BUILD_TYPES})
        string(TOUPPER ${TYPE} TYPE_UPPER)
        set(FLAG_VAR "CMAKE_${LANG}_FLAGS_${TYPE_UPPER}")

        set(${FLAG_VAR} ${COMPILE_FLAGS_${TYPE_UPPER}} CACHE STRING "${TYPE} flags")
        mark_as_advanced(${FLAG_VAR})
    endforeach()
endforeach()

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES ${BUILD_TYPES} CACHE STRING "Supported build types" FORCE)
    mark_as_advanced(CMAKE_CONFIGURATION_TYPES)
else()
    set(CMAKE_BUILD_TYPE "DebugOptimized" CACHE STRING "Project build type")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
endif()

# Don't clutter GUI on success
mark_as_advanced(
    N64_TOOLCHAIN_ROOT
    N64_TOOLCHAIN_PREFIX

    CMAKE_ASM_COMPILER
    CMAKE_C_COMPILER
    CMAKE_CPP
    CMAKE_CXX_COMPILER
    CMAKE_OBJCOPY
    Makemask_EXECUTABLE
)

# ROM generation

function(target_linker_script TARGET SCRIPT_FILE)
    set_target_properties(${TARGET} PROPERTIES
        LINK_DEPENDS "${SCRIPT_FILE}"
    )
    target_link_options(${TARGET} PRIVATE
        -T "${SCRIPT_FILE}"
        -Wl,-Map=$<PATH:REPLACE_EXTENSION,$<TARGET_FILE:${TARGET}>,map>
    )
endfunction()

function(add_n64_rom TARGET)
    set(INPUT_FILE "$<TARGET_FILE:${TARGET}>")
    set(OUTPUT_ROM "$<PATH:REPLACE_EXTENSION,$<TARGET_FILE:${TARGET}>,z64>")

    add_custom_command(TARGET portal POST_BUILD
        COMMAND
            ${CMAKE_OBJCOPY} --pad-to=0x100000 --gap-fill=0xFF -O binary
            ${INPUT_FILE}
            ${OUTPUT_ROM}
        COMMAND
            ${Makemask_EXECUTABLE}
            ${OUTPUT_ROM}
        COMMENT
            "Generating $<PATH:RELATIVE_PATH,${OUTPUT_ROM},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )
endfunction()
