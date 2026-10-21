add_compile_options(
    -O3 -march=native -fPIC -w
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/symmetric
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/api)

aux_source_directory(./symmetric SYM_SRC)
aux_source_directory(./backend BAK_SRC)
aux_source_directory(./api API_SRC)

set(LIB_NAME Amoeba${SECURITY_LEVEL})
add_library(${LIB_NAME} SHARED ${SYM_SRC} ${BAK_SRC} ${API_SRC})
target_compile_definitions(${LIB_NAME} PUBLIC SECURITY_LEVEL=${SECURITY_LEVEL})

set(TEST_CPAPKE_NAME TEST_CPAPKE_${SECURITY_LEVEL})
set(TEST_CCAKEM_NAME TEST_CCAKEM_${SECURITY_LEVEL})
add_subdirectory(tests)

set(KAT_KEM_NAME KAT_KEM_${SECURITY_LEVEL})
add_subdirectory(KAT)
