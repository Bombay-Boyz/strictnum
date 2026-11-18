cmake_minimum_required(VERSION 3.15)

# Single source of truth for the version: the STRICTNUM_VERSION_* macros in
# strictnum.h.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/strictnum.h" _strictnum_version_lines
     REGEX "^#define STRICTNUM_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+")
foreach(_part MAJOR MINOR PATCH)
    set(_strictnum_${_part} "")
    foreach(_line IN LISTS _strictnum_version_lines)
        if(_line MATCHES "STRICTNUM_VERSION_${_part}[ \t]+([0-9]+)")
            set(_strictnum_${_part} "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(_strictnum_${_part} STREQUAL "")
        message(FATAL_ERROR "strictnum: cannot read STRICTNUM_VERSION_${_part} from strictnum.h")
    endif()
endforeach()

project(strictnum
    VERSION ${_strictnum_MAJOR}.${_strictnum_MINOR}.${_strictnum_PATCH}
    LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

get_directory_property(_strictnum_is_subproject PARENT_DIRECTORY)
if(_strictnum_is_subproject)
    set(_strictnum_default_install OFF)
    set(_strictnum_default_tests OFF)
else()
    set(_strictnum_default_install ON)
    set(_strictnum_default_tests ON)
endif()
option(STRICTNUM_INSTALL "Install strictnum and its CMake package" ${_strictnum_default_install})
option(STRICTNUM_BUILD_TESTS "Build strictnum's CTest tests" ${_strictnum_default_tests})

add_library(strictnum INTERFACE)
add_library(strictnum::strictnum ALIAS strictnum)
target_include_directories(strictnum INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/strictnum>
)

if(STRICTNUM_INSTALL)
    install(TARGETS strictnum EXPORT strictnumTargets)
    # One installed copy, in a namespaced directory. Both `#include
    # "strictnum.h"` (through the CMake target or pkg-config) and
    # `#include <strictnum/strictnum.h>` (with the prefix include directory
    # on the search path) work.
    install(FILES strictnum.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/strictnum")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/strictnum.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/strictnum.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/strictnum.pc"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pkgconfig")
    install(EXPORT strictnumTargets
        FILE strictnumTargets.cmake
        NAMESPACE strictnum::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/strictnum"
    )
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/strictnumConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/strictnumConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/strictnum"
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/strictnumConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/strictnumConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/strictnumConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/strictnum"
    )
endif()

if(STRICTNUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
