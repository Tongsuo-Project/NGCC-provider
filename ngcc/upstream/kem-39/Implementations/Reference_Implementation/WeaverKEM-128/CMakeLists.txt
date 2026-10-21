# CMakeLists.txt for WeaverKEM-128 (Reference Implementation)
# Subdirectory configuration - invoked from parent CMakeLists.txt

add_compile_definitions(WEAVER_MODE=1)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 17 files
set(SOURCES
    drng.c
    auxfunc.c
    KAT_KEM.c
    KEM_WeaverKEM-128.c
    symmetric-iccs.c
    kem_cca.c
    indcpa.c
    polyvec.c
    poly.c
    ntt.c
    cbd.c
    reduce.c
    verify.c
    msgenc.c
    bch_high.c
    bch_low.c
    poly_invq.c
)

add_executable(KAT_KEM_WeaverKEM-128 ${SOURCES})

if(UNIX)
    target_link_libraries(KAT_KEM_WeaverKEM-128 m)
endif()

add_custom_target(generate_kat_128
    COMMAND KAT_KEM_WeaverKEM-128
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating KAT_KEM_WeaverKEM-128.txt"
    DEPENDS KAT_KEM_WeaverKEM-128
)