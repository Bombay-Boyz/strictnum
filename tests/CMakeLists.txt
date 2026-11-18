# CTest suite. Each test is one executable built against the strictnum target.
function(strictnum_test name)
    cmake_parse_arguments(T "" "" "SOURCES;DEFINES;OPTIONS" ${ARGN})
    add_executable(strictnum_${name} ${T_SOURCES})
    target_link_libraries(strictnum_${name} PRIVATE strictnum::strictnum)
    if(T_DEFINES)
        target_compile_definitions(strictnum_${name} PRIVATE ${T_DEFINES})
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(strictnum_${name} PRIVATE -Wall -Wextra -Wpedantic ${T_OPTIONS})
    elseif(MSVC)
        target_compile_options(strictnum_${name} PRIVATE /W4)
    endif()
    add_test(NAME strictnum.${name} COMMAND strictnum_${name})
endfunction()

strictnum_test(core SOURCES test_strictnum.c)
strictnum_test(core_no64 SOURCES test_strictnum.c DEFINES STRICTNUM_NO_64)
strictnum_test(portable SOURCES test_portable.c)
strictnum_test(portable_no_stdint_no64 SOURCES test_portable.c
    DEFINES STRICTNUM_NO_STDINT STRICTNUM_NO_64)
strictnum_test(digit_fallback SOURCES test_portable.c
    DEFINES STRICTNUM_TEST_FORCE_NONASCII_DIGITS)
strictnum_test(hygiene SOURCES header_hygiene.c)
strictnum_test(multi_tu
    SOURCES multi_tu/first.c multi_tu/second.c multi_tu/main.c)

# Not built with -Wpedantic: this test deliberately defines SN_API itself.
add_executable(strictnum_namespace test_header_namespace.c)
target_link_libraries(strictnum_namespace PRIVATE strictnum::strictnum)
add_test(NAME strictnum.namespace COMMAND strictnum_namespace)
