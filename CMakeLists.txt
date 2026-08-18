cmake_minimum_required(VERSION 3.10)
project(kdf VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(kdf
    src/kdf_document.c
    src/kdf_value.c
    src/kdf_object.c
    src/kdf_array.c
    src/kdf_io.c
    src/kdf_text_writer.c
    src/kdf_text_parser.c
    src/kdf_binary_writer.c
    src/kdf_binary_reader.c
)

target_include_directories(kdf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(kdf PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<C_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<C_COMPILER_ID:MSCL>:/W4>
)

# Tests
option(KDF_BUILD_TESTS "Build tests" ON)
if(KDF_BUILD_TESTS)
    enable_testing()
    add_executable(kdf_tests tests/test_kdf.c)
    target_link_libraries(kdf_tests PRIVATE kdf m)
    add_test(NAME kdf_tests COMMAND kdf_tests)
endif()
