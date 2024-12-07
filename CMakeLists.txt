#
# project: chips-test
#
cmake_minimum_required(VERSION 3.20)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
project(chips-test)

# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup()
fips_import_dcimgui_dcimgui()

# just suppress this pesky "missing field initializer warning" for now
if (FIPS_CLANG OR FIPS_GCC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers")
endif()
if (NOT FIPS_EMSCRIPTEN AND NOT FIPS_ANDROID)
    fips_ide_group(tests)
    add_subdirectory(tests)
endif()
fips_ide_group(examples)
add_subdirectory(examples)
fips_ide_group(tools)
add_subdirectory(tools)
