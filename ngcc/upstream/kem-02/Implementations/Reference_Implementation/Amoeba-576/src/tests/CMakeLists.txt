set(PROFILING_ENABLE OFF)

if (PROFILING_ENABLE)
    add_compile_definitions(PROFILING_ENABLE)
    set(PROF_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/prof")
    add_compile_definitions(PROFILE_PATH="${PROF_OUTPUT_DIR}/")
    file(MAKE_DIRECTORY ${PROF_OUTPUT_DIR})
endif()

add_executable(${TEST_CPAPKE_NAME} test_cpapke.c)
target_link_libraries(${TEST_CPAPKE_NAME} PRIVATE ${LIB_NAME})

add_executable(${TEST_CCAKEM_NAME} test_ccakem.c)
target_link_libraries(${TEST_CCAKEM_NAME} PRIVATE ${LIB_NAME})

if (PROFILING_ENABLE)
    target_link_libraries(${TEST_CPAPKE_NAME} PRIVATE profiler)
    target_link_libraries(${TEST_CCAKEM_NAME} PRIVATE profiler)
endif()
