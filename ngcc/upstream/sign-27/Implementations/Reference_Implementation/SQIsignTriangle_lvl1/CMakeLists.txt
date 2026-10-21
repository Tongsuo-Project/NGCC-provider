cmake_minimum_required(VERSION 3.13)
project(SQIsignTriangle_lvl1_API_PKC C ASM)

set(CMAKE_C_STANDARD 11)

# Bundled SQIsignTriangle implementation, built as the "ref" backend.
set(SQISIGN_ROOT "${CMAKE_CURRENT_LIST_DIR}/sqisigntriangle" CACHE PATH "Path to the SQIsignTriangle C implementation")
set(SQISIGN_BUILD_TYPE ref CACHE STRING "SQIsignTriangle implementation type")
set(GMP_LIBRARY SYSTEM CACHE STRING "GMP backend")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
set(ENABLE_TESTS OFF CACHE BOOL "Disable SQIsign internal test targets in this wrapper build")

# Expose the ICCS auxiliary header to the bundled sources so the SM3-based
# fips202.c adapter can include "auxfunc.h".
include_directories("${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory("${SQISIGN_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/sqisigntriangle")

# KAT generator. auxfunc.c supplies the ICCS SM3 hash/XOF used by fips202.c.
add_executable(KAT_SIG_SQIsignTriangle_lvl1
	KAT_SIG.c
	SIG_AlgorithmInstance.c
	drng.c
	auxfunc.c
)

target_include_directories(KAT_SIG_SQIsignTriangle_lvl1 PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}"
	"${SQISIGN_ROOT}/include"
	"${SQISIGN_ROOT}/src/nistapi/lvl1"
)

target_compile_definitions(KAT_SIG_SQIsignTriangle_lvl1 PRIVATE
	ENABLE_SIGN
	SQISIGN_VARIANT=lvl1
	SQISIGN_BUILD_TYPE_REF
)

target_link_libraries(KAT_SIG_SQIsignTriangle_lvl1 PRIVATE sqisign_lvl1_test_nistapi)
