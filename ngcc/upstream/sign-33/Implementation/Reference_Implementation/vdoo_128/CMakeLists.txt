cmake_minimum_required(VERSION 3.11.4)
project(VDOO_Reference_Implementation C)

set(VDOO_LEVEL 128)
set(VDOO_DEF "-DVDOO_${VDOO_LEVEL}")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-std=c99 -Wpedantic -Wall -Wextra -O2 ${VDOO_DEF}")

set(LIB_SOURCES
        auxfunc.c
        blas_comm.c
        drng.c
        parallel_matrix_op.c
        rng.c
        utils.c
        api.c
        SIG_AlgorithmInstance.c
        vdoo_keypair.c
        vdoo_sign.c
        vdoo_verif.c
)

add_library(vdoo STATIC ${LIB_SOURCES})
target_include_directories(vdoo PUBLIC .)


# KAT_SIG
add_executable(KAT_SIG KAT_SIG.c)
target_link_libraries(KAT_SIG vdoo)


# vdoo_bins
add_executable(vdoo-keygen vdoo-keygen.c)
target_link_libraries(vdoo-keygen vdoo)

add_executable(vdoo-sign vdoo-sign.c)
target_link_libraries(vdoo-sign vdoo)

add_executable(vdoo-verif vdoo-verif.c)
target_link_libraries(vdoo-verif vdoo)

add_custom_target(vdoo_bins DEPENDS vdoo-keygen vdoo-sign vdoo-verif)


# bench_speed
add_executable(bench_speed
        ${LIB_SOURCES}
        test/cpucycles.c
        test/speed_print.c
        test/test_speed.c
)
target_include_directories(bench_speed PRIVATE . test)
target_link_libraries(bench_speed)


# test_memory (Resource Consumption Testing)
add_executable(test_memory
        ${LIB_SOURCES}
        test/test_memory.c
)
target_include_directories(test_memory PRIVATE . test)
target_link_libraries(test_memory)


# test_sizes (Transmission and Storage Overhead Testing)
add_executable(test_sizes
        ${LIB_SOURCES}
        test/test_sizes.c
)
target_include_directories(test_sizes PRIVATE . test)
target_link_libraries(test_sizes)