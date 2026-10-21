cmake_minimum_required(VERSION 3.16)
project(TRIKE-1 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB TRIKE_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/ICCS/*.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(trike1 STATIC ${TRIKE_SOURCES})
target_include_directories(trike1 PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/ICCS"
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_executable(kat_kem "${CMAKE_CURRENT_SOURCE_DIR}/tests/KAT_KEM.c")
target_link_libraries(kat_kem PRIVATE trike1)
