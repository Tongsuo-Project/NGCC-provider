cmake_minimum_required(VERSION 3.10)
project(uvw_kem_256)


set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto -funroll-all-loops")


include_directories(include)


add_executable(KAT_KEM
    src/KAT_KEM.c
    src/KEM_AlgorithmInstance.c 
    src/drng.c
    src/gf_math.c
    src/auxfunc.c
    src/list_decoding.c
    src/uvw_constants.c
)



