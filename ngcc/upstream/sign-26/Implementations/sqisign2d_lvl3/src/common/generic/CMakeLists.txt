set(SOURCE_FILES_COMMON_SYS 
    randombytes_system.c 
    aes_c.c 
    fips202.c 
    xof_shake.c
    mem.c
    tools.c
)

add_library(sqisign_common_sys ${SOURCE_FILES_COMMON_SYS})
target_include_directories(sqisign_common_sys PRIVATE include ../../include)
target_compile_options(sqisign_common_sys PUBLIC ${C_OPT_FLAGS})

set(SOURCE_FILES_COMMON_TEST 
    randombytes_ctrdrbg.c 
    aes_c.c 
    fips202.c 
    xof_shake.c
    mem.c
    tools.c
)

add_library(sqisign_common_test ${SOURCE_FILES_COMMON_TEST})
target_include_directories(sqisign_common_test PRIVATE include ../include)
target_compile_options(sqisign_common_test PUBLIC ${C_OPT_FLAGS})

if(ENABLE_ICCS_KAT)
    set(SOURCE_FILES_COMMON_ICCS
        ${PROJECT_SOURCE_DIR}/src/iccs/randombytes_iccs.c
        ${PROJECT_SOURCE_DIR}/src/iccs/xof_iccs.c
        ${PROJECT_SOURCE_DIR}/src/iccs/auxfunc.c
        mem.c
        tools.c
    )

    add_library(sqisign_common_iccs ${SOURCE_FILES_COMMON_ICCS})
    target_include_directories(
        sqisign_common_iccs
        PRIVATE
            include
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/src/iccs
    )
    target_compile_options(sqisign_common_iccs PUBLIC ${C_OPT_FLAGS})
endif()

if (ENABLE_CT_TESTING)
    target_compile_definitions(sqisign_common_sys PUBLIC ENABLE_CT_TESTING)
    target_compile_definitions(sqisign_common_test PUBLIC ENABLE_CT_TESTING)
endif()
