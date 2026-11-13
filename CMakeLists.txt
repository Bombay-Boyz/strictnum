cmake_minimum_required(VERSION 3.15)
project(strictnum VERSION 1.0.0 LANGUAGES C)

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
    # Install both the conventional top-level header and a namespaced path.
    install(FILES strictnum.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(FILES strictnum.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/strictnum")
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
    add_executable(strictnum_test_strict tests/test_strictnum.c)
    target_link_libraries(strictnum_test_strict PRIVATE strictnum::strictnum)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(strictnum_test_strict PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME strictnum.core COMMAND strictnum_test_strict)

    add_executable(strictnum_test_portable tests/test_portable.c)
    target_link_libraries(strictnum_test_portable PRIVATE strictnum::strictnum)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(strictnum_test_portable PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME strictnum.portable COMMAND strictnum_test_portable)

    add_executable(strictnum_test_namespace tests/test_header_namespace.c)
    target_link_libraries(strictnum_test_namespace PRIVATE strictnum::strictnum)
    add_test(NAME strictnum.namespace COMMAND strictnum_test_namespace)
endif()
