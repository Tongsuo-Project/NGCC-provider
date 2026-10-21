foreach(SVARIANT ${SVARIANT_S})
    string(TOLOWER ${SVARIANT} SVARIANT_LOWER)
    string(TOUPPER ${SVARIANT} SVARIANT_UPPER)

    set(ICCS_KAT_TARGET kat_sig_sqisign2d_${SVARIANT_LOWER})

    add_executable(
        ${ICCS_KAT_TARGET}
        KAT_SIG.c
        SIG_AlgorithmInstance.c
        drng.c
    )

    target_link_libraries(
        ${ICCS_KAT_TARGET}
        PRIVATE
            sqisign_${SVARIANT_LOWER}_iccs
    )

    target_include_directories(
        ${ICCS_KAT_TARGET}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${INC_PUBLIC}
            ${INC_PRECOMP_${SVARIANT_UPPER}}
    )

    target_compile_definitions(
        ${ICCS_KAT_TARGET}
        PRIVATE
            OUTPUT_BLANK_TEST_VECTORS=0
            "ALGORITHM_INSTANCE=\"SQIsign2D-${SVARIANT_LOWER}\""
    )

    set_target_properties(
        ${ICCS_KAT_TARGET}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY
            ${CMAKE_BINARY_DIR}/iccs/${SVARIANT_LOWER}
    )

    add_test(
        NAME
            iccs_kat_${SVARIANT_LOWER}
        COMMAND
            ${ICCS_KAT_TARGET}
    )

    set_tests_properties(
        iccs_kat_${SVARIANT_LOWER}
        PROPERTIES
            WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}/iccs/${SVARIANT_LOWER}
            LABELS
            "iccs;kat;${SVARIANT_LOWER}"
    )
endforeach()
