cmake_minimum_required(VERSION 3.5)
project(PIKE VERSION 1.0 LANGUAGES C)

set(PIKE_VERSION "0")
set(CMAKE_C_STANDARD 99)

include(CTest)

option(ENABLE_STRICT "Build with strict compile options." OFF)
option(ENABLE_TESTS  "Enable compilation of tests." ON)
option(ENABLE_CT_TESTING  "Enable compilation for constant time testing." OFF)
option(ENABLE_GMP_BUILD "Download and build external version of GMP" OFF)
option(ENABLE_COMPRESSED "Build compressed variant of PIKE." ON)
set(PIKE_NGCC_XOF_BACKEND "sm3" CACHE STRING "XOF/hash backend for NGCC PIKE builds: shake or sm3.")
set_property(CACHE PIKE_NGCC_XOF_BACKEND PROPERTY STRINGS shake sm3)

if(PIKE_NGCC_XOF_BACKEND STREQUAL "shake")
    add_compile_definitions(PIKE_XOF_BACKEND=1)
elseif(PIKE_NGCC_XOF_BACKEND STREQUAL "sm3")
    add_compile_definitions(PIKE_XOF_BACKEND=2)
else()
    message(FATAL_ERROR "Unsupported PIKE_NGCC_XOF_BACKEND: ${PIKE_NGCC_XOF_BACKEND}. Use 'shake' or 'sm3'.")
endif()
message(STATUS "PIKE NGCC XOF/hash backend: ${PIKE_NGCC_XOF_BACKEND}")

if (NOT DEFINED PIKE_BUILD_TYPE)
	SET(PIKE_BUILD_TYPE "ref")
endif()

SET(SVARIANT_S "NGCC_1;NGCC_2;NGCC_3")

set(SELECT_PIKE_VARIANT ${PROJECT_SOURCE_DIR}/.cmake/pike_variant.cmake)
set(SELECT_IMPL_TYPE ${PROJECT_SOURCE_DIR}/.cmake/impl_type.cmake)

set(INC_PUBLIC ${PROJECT_SOURCE_DIR}/include)

include(.cmake/flags.cmake)
include(.cmake/sanitizers.cmake)
include(.cmake/target.cmake)
include(.cmake/gmpconfig.cmake)

add_subdirectory(src)
