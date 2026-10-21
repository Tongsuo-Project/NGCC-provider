# There are the following dependencies
#     ┌─┬──────┬─┐           ┌─┬────┬─┐            ┌─┬──────┬─┐
#     │ ├──────┤ │           │ ├────┤ │            │ ├──────┤ │
#     │ │Keygen│ │           │ │Sign│ │            │ │Verify│ │
#     │ ├──────┤ │           │ ├────┤ │            │ ├──────┤ │
#     └─┴───┬──┴─┘           └─┴─┬──┴─┘            └─┴───┬──┴─┘
#           │                    │                       │
#           │                    │                       │
#           ├────────────────────┼─────────────────┐     │
#           │                    │                 │     │
#           │                    │                 │     │
#       ┌───▼──┐          ┌──────▼────────┐   ┌────▼─────▼───────────┐
#       │ PRNG ◄────┬─────┤ Iso <-> Ideal ├───►   Elliptic Curves,   │
#       └───▲──┘    │     └──────┬────────┘   │ Pairings & Isogenies │
#           │       │            │            └───▲──────┬───────────┘
#           │       │            │                │      │
#       ┌───┴──┐    │            │                │      │
#       │ KLPT ◄────┘            │     ┌──────────┘      │
#       └───┬──┘                 │     │                 │
#           │                    │     │                 │
# ┌─────────▼─────────┐          │     │                 │
# │ Quaternion orders │          │     │            ┌────▼───┐
# │     and ideals    │          │     │            │ GF(p²) │
# └─────────┬─────────┘          │     │            └────┬───┘
#           │           ┌─┬──────▼─────┴──┬─┐            │
#     ┌─────▼─────┐     │ ├───────────────┤ │      ┌─────▼─────┐
#     │ MP BigInt │     │ │Precomputations│ │      │ FP BigInt │
#     └───────────┘     │ ├───────────────┤ │      └───────────┘
#                       └─┴───────────────┴─┘                    

add_subdirectory(common)
add_subdirectory(intbig)
add_subdirectory(quaternion)
add_subdirectory(precomp)
add_subdirectory(klpt)
add_subdirectory(gf)
add_subdirectory(ec)


add_subdirectory(id2iso)
add_subdirectory(hd)
add_subdirectory(dim2id2iso)
add_subdirectory(sqisigndim2)
FOREACH(SVARIANT ${SVARIANT_S})
    string(TOLOWER ${SVARIANT} SVARIANT_LOWER)
    string(TOUPPER ${SVARIANT} SVARIANT_UPPER)
    set(SOURCE_FILES_VARIANT sqisign.c)
    # Library for SQIsign variant
    add_library(sqisign_${SVARIANT_LOWER} ${SOURCE_FILES_VARIANT})
    target_link_libraries(sqisign_${SVARIANT_LOWER} PUBLIC 
        ${LIB_PROTOCOLS_${SVARIANT_UPPER}}
        ${LIB_SQISIGNDIM2_${SVARIANT_UPPER}}
        ${LIB_DIM2ID2ISO_${SVARIANT_UPPER}} 
        ${LIB_ID2ISO_${SVARIANT_UPPER}}  
        ${LIB_KLPT_${SVARIANT_UPPER}} 
        ${LIB_QUATERNION} 
        ${LIB_PRECOMP_${SVARIANT_UPPER}} 
        ${LIB_INTBIG} 
        ${LIB_GF_${SVARIANT_UPPER}} 
        ${LIB_EC_${SVARIANT_UPPER}}
        ${LIB_HD_${SVARIANT_UPPER}} 
        ${GMP} 
        sqisign_common_sys
    )

    target_include_directories(sqisign_${SVARIANT_LOWER} PUBLIC ${INC_PROTOCOLS} ${INC_INTBIG} ${INC_QUATERNION} ${INC_PRECOMP_${SVARIANT_UPPER}} ${INC_EC} ${INC_GF} ${INC_GF_${SVARIANT_UPPER}} ${INC_COMMON} ${INC_KLPT} ${INC_HD} ${INC_ID2ISO} ${INC_DIM2ID2ISO} ${INC_SQISIGNDIM2} ../include PRIVATE common/generic internal)
    target_compile_definitions(sqisign_${SVARIANT_LOWER} PUBLIC SQISIGN_VARIANT=${SVARIANT})

    # Library for SQIsign variant (test)
    add_library(sqisign_${SVARIANT_LOWER}_test ${SOURCE_FILES_VARIANT})
    target_link_libraries(sqisign_${SVARIANT_LOWER}_test PUBLIC 
        ${LIB_PROTOCOLS_${SVARIANT_UPPER}}
        ${LIB_SQISIGNDIM2_${SVARIANT_UPPER}}
        ${LIB_DIM2ID2ISO_${SVARIANT_UPPER}} 
        ${LIB_ID2ISO_${SVARIANT_UPPER}} 
        ${LIB_KLPT_${SVARIANT_UPPER}} 
        ${LIB_QUATERNION} 
        ${LIB_PRECOMP_${SVARIANT_UPPER}} 
        ${LIB_INTBIG} 
        ${LIB_GF_${SVARIANT_UPPER}} 
        ${LIB_EC_${SVARIANT_UPPER}}
        ${LIB_HD_${SVARIANT_UPPER}} 
        ${GMP} 
        sqisign_common_test
    )

    target_include_directories(sqisign_${SVARIANT_LOWER}_test PUBLIC ${INC_PROTOCOLS} ${INC_INTBIG} ${INC_QUATERNION} ${INC_PRECOMP_${SVARIANT_UPPER}} ${INC_EC} ${INC_GF} ${INC_GF_${SVARIANT_UPPER}} ${INC_COMMON} ${INC_KLPT} ${INC_HD} ${INC_ID2ISO} ${INC_DIM2ID2ISO} ${INC_SQISIGNDIM2} ../include PRIVATE common/generic internal)
    target_compile_definitions(sqisign_${SVARIANT_LOWER}_test PUBLIC SQISIGN_VARIANT=${SVARIANT})

    if(ENABLE_ICCS_KAT)
        add_library(sqisign_${SVARIANT_LOWER}_iccs ${SOURCE_FILES_VARIANT})
        target_link_libraries(sqisign_${SVARIANT_LOWER}_iccs PUBLIC 
            ${LIB_PROTOCOLS_${SVARIANT_UPPER}}
            ${LIB_SQISIGNDIM2_${SVARIANT_UPPER}}
            ${LIB_DIM2ID2ISO_${SVARIANT_UPPER}} 
            ${LIB_ID2ISO_${SVARIANT_UPPER}}  
            ${LIB_KLPT_${SVARIANT_UPPER}} 
            ${LIB_QUATERNION} 
            ${LIB_PRECOMP_${SVARIANT_UPPER}} 
            ${LIB_INTBIG} 
            ${LIB_GF_${SVARIANT_UPPER}} 
            ${LIB_EC_${SVARIANT_UPPER}}
            ${LIB_HD_${SVARIANT_UPPER}} 
            ${GMP} 
            sqisign_common_iccs
        )
        target_include_directories(
            sqisign_${SVARIANT_LOWER}_iccs
            PUBLIC
                ${INC_PROTOCOLS}
                ${INC_INTBIG}
                ${INC_QUATERNION}
                ${INC_PRECOMP_${SVARIANT_UPPER}}
                ${INC_EC}
                ${INC_GF}
                ${INC_GF_${SVARIANT_UPPER}}
                ${INC_COMMON}
                ${INC_KLPT}
                ${INC_HD}
                ${INC_ID2ISO}
                ${INC_DIM2ID2ISO}
                ${INC_SQISIGNDIM2}
                ../include
            PRIVATE
                common/generic
                internal
        )
        target_compile_definitions(sqisign_${SVARIANT_LOWER}_iccs PUBLIC SQISIGN_VARIANT=${SVARIANT})
    endif()

    add_executable(test_sqisign_api_${SVARIANT_LOWER} test/test_sqisign_api.c)
    target_link_libraries(test_sqisign_api_${SVARIANT_LOWER} sqisign_${SVARIANT_LOWER}_test)
    target_include_directories(test_sqisign_api_${SVARIANT_LOWER} PRIVATE ${INC_PUBLIC} ${INC_PRECOMP_${SVARIANT_UPPER}})
    add_test(test_sqisign_api_${SVARIANT_LOWER} test_sqisign_api_${SVARIANT_LOWER})

    add_executable(test_protocol_roundtrip_${SVARIANT_LOWER} sqisigndim2/ref/sqisigndim2x/test/test_protocol_roundtrip.c)
    target_link_libraries(test_protocol_roundtrip_${SVARIANT_LOWER} sqisign_${SVARIANT_LOWER}_test)
    add_test(test_protocol_roundtrip_${SVARIANT_LOWER} test_protocol_roundtrip_${SVARIANT_LOWER})

    add_executable(test_sqisign_serialization_layers_${SVARIANT_LOWER} test/test_sqisign_serialization_layers.c)
    target_link_libraries(test_sqisign_serialization_layers_${SVARIANT_LOWER} sqisign_${SVARIANT_LOWER}_test)
    add_test(test_sqisign_serialization_layers_${SVARIANT_LOWER} test_sqisign_serialization_layers_${SVARIANT_LOWER})

ENDFOREACH()

if(ENABLE_ICCS_KAT)
    add_subdirectory(iccs)
endif()
