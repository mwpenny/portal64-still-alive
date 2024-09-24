set(CMAKE_SYSTEM_NAME Generic)

set(N64_TOOLCHAIN_PREFIX "" CACHE PATH "Root directory of N64 toolchain")
set(N64_BIN_DIR "${N64_TOOLCHAIN_PREFIX}/bin")

# Search for programs in the host environment
# Search for headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH "${N64_TOOLCHAIN_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Ensure CMake can find toolchain during compiler tests
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES N64_TOOLCHAIN_PREFIX)

# Don't try to dynamically link during compiler tests
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# TODO: find assembler, linker, etc.
# TODO: set flags
find_program(CMAKE_C_COMPILER mips64-elf-gcc PATHS ${N64_BIN_DIR} REQUIRED)
