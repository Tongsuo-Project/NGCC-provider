cmake_minimum_required(VERSION 3.15)

get_filename_component(BAG_PIGLET_INSTANCE "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${BAG_PIGLET_INSTANCE} LANGUAGES C)

include(CheckIPOSupported)
include(CheckCCompilerFlag)
check_ipo_supported(RESULT BAG_PIGLET_IPO_SUPPORTED OUTPUT BAG_PIGLET_IPO_ERROR)
if(BAG_PIGLET_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i.86)$")
    check_c_compiler_flag("-mpclmul" BAG_PIGLET_HAS_MPCLMUL)
    if(BAG_PIGLET_HAS_MPCLMUL)
        add_compile_options(-mpclmul)
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-sign-compare")

option(
    BAG_PIGLET_BUILD_LOCAL_TESTS
    "Build local-only functional and benchmark programs when their sources exist"
    ON
)

set(COMMON_DIR src/common)
set(SCHEME_DIR src/scheme)
set(FFI_DIR src/ffi)
set(RBC_DIR src/rbc)
set(KAT_DIR src/kat)
set(API_PKC_DIR lib/api_pkc)
set(RANDOMBYTES_DIR lib/randombytes)

file(GLOB COMMON_SRC ${COMMON_DIR}/*.c)
file(GLOB SCHEME_SRC ${SCHEME_DIR}/*.c)
file(GLOB FFI_SRC ${FFI_DIR}/*.c)
file(GLOB RBC_SRC ${RBC_DIR}/*.c)
file(GLOB API_PKC_SRC ${API_PKC_DIR}/*.c)
set(RANDOMBYTES_SRC ${RANDOMBYTES_DIR}/randombytes.c)

# The generic KEM adapter belongs to the local test programs. The official KAT
# uses src/kat/KEM_AlgorithmInstance.c and must not link both adapters.
list(FILTER COMMON_SRC EXCLUDE REGEX ".*/kem_bag_piglet\\.c$")

set(BAG_PIGLET_CORE
    ${COMMON_SRC}
    ${SCHEME_SRC}
    ${FFI_SRC}
    ${RBC_SRC}
    ${API_PKC_SRC}
    ${RANDOMBYTES_SRC}
)

function(add_bag_piglet_executable target)
    add_executable(${target} ${ARGN} ${BAG_PIGLET_CORE})
    target_include_directories(${target} PRIVATE
        ${COMMON_DIR}
        ${SCHEME_DIR}
        ${FFI_DIR}
        ${RBC_DIR}
        ${KAT_DIR}
        ${API_PKC_DIR}
        ${RANDOMBYTES_DIR}
    )
    target_link_libraries(${target} PRIVATE pthread)
endfunction()

set(BAG_PIGLET_LOCAL_ADAPTER ${COMMON_DIR}/kem_bag_piglet.c)

if(BAG_PIGLET_BUILD_LOCAL_TESTS
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main_bag_piglet.c"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BAG_PIGLET_LOCAL_ADAPTER}")
    add_bag_piglet_executable(
        ${BAG_PIGLET_INSTANCE}
        src/main_bag_piglet.c
        ${BAG_PIGLET_LOCAL_ADAPTER}
    )
endif()

if(BAG_PIGLET_BUILD_LOCAL_TESTS
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main_benchmark.c"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BAG_PIGLET_LOCAL_ADAPTER}")
    add_bag_piglet_executable(
        ${BAG_PIGLET_INSTANCE}-bench
        src/main_benchmark.c
        ${BAG_PIGLET_LOCAL_ADAPTER}
    )
endif()

add_bag_piglet_executable(${BAG_PIGLET_INSTANCE}-kat
    ${KAT_DIR}/KAT_KEM.c
    ${KAT_DIR}/KEM_AlgorithmInstance.c
)
target_compile_definitions(${BAG_PIGLET_INSTANCE}-kat PRIVATE BAG_PIGLET_KAT=1)
