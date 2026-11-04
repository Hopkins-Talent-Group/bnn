cmake_minimum_required(VERSION 3.14)
project(nanolm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Strict warning flags — must compile clean before any claim is accepted.
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Wno-missing-field-initializers
    )
endif()

add_executable(nanolm nanolm.cpp)

# Inject git commit hash at build time so benchmark/log output is traceable.
execute_process(
    COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    ERROR_QUIET
)
if(GIT_COMMIT)
    target_compile_definitions(nanolm PRIVATE BNN_SOURCE_COMMIT="${GIT_COMMIT}")
endif()

include(CTest)
if(BUILD_TESTING)
    # Smoke test: nanolm must exit cleanly when given no arguments (prints usage).
    add_test(NAME nanolm_no_args COMMAND nanolm)
    set_tests_properties(nanolm_no_args PROPERTIES WILL_FAIL FALSE)
endif()
