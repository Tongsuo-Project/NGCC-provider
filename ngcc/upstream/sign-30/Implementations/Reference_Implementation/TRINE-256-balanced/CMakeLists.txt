cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER gcc)
endif()

project(Trine C)

find_package(OpenSSL REQUIRED)

if(DEBUG)
  set(COMMON_COMPILE_OPTIONS -g -Wall)
  set(COMMON_COMPILE_DEFINITIONS DEBUG)
else()
  set(COMMON_COMPILE_OPTIONS -O3 -Wall)
  set(COMMON_COMPILE_DEFINITIONS)
endif()

if(LOG)
  list(APPEND COMMON_COMPILE_DEFINITIONS DEBUG)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(TRINE_CORE_SOURCES
  trine.c
  util.c
  osfreq.c
  field.c
  matrixmod.c
  matrixelim.c
  triform.c
  canonical.c
  corank1.c
  trine_expand.c
  trine_codec.c
  bitstream.c
  hashkdf.c
)

set(TRINE_NORMAL_RANDOM_SOURCES
  randombytes.c
)

set(TRINE_ICCS_RANDOM_ADAPTER_SOURCES
  randombytes_iccs.c
)

set(TRINE_SHA3_HASH_XOF_SOURCES
  fips202.c
)

set(TRINE_ICCS_HASH_XOF_SOURCES
  iccs/auxfunc.c
)

set(TRINE_ICCS_DRNG_SOURCES
  iccs/drng.c
)

set(TRINE_ICCS_KAT_SOURCES
  iccs/KAT_SIG.c
  iccs/SIG_AlgorithmInstance.c
)

function(add_trine_parameter_set suffix param_id algorithm_instance)
  set(kat_target "kat_${suffix}")
  set(speed_target "speed_${suffix}")

  add_executable(${kat_target}
    ${TRINE_CORE_SOURCES}
    ${TRINE_ICCS_RANDOM_ADAPTER_SOURCES}
    ${TRINE_ICCS_KAT_SOURCES}
    ${TRINE_ICCS_DRNG_SOURCES}
    ${TRINE_ICCS_HASH_XOF_SOURCES}
  )
  target_compile_options(${kat_target} PRIVATE ${COMMON_COMPILE_OPTIONS})
  target_compile_definitions(${kat_target}
    PRIVATE
      PARAMS=${param_id}
      USE_ICCS
      TRINE_ALGORITHM_INSTANCE="${algorithm_instance}"
      ${COMMON_COMPILE_DEFINITIONS}
  )
  target_include_directories(${kat_target} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/iccs"
  )

  add_executable(${speed_target}
    ${TRINE_CORE_SOURCES}
    ${TRINE_NORMAL_RANDOM_SOURCES}
    ${TRINE_SHA3_HASH_XOF_SOURCES}
    test_speed/test_speed.c
    test_speed/cpucycles.c
    test_speed/speed_print.c
  )
  target_compile_options(${speed_target} PRIVATE ${COMMON_COMPILE_OPTIONS})
  target_compile_definitions(${speed_target}
    PRIVATE
      PARAMS=${param_id}
      USE_SHA3
      ${COMMON_COMPILE_DEFINITIONS}
  )
  target_include_directories(${speed_target} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_speed"
  )
  target_link_libraries(${speed_target} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endfunction()

add_trine_parameter_set(balanced_i 1 "TRINE-Balanced-I")
add_trine_parameter_set(balanced_iii 2 "TRINE-Balanced-III")
add_trine_parameter_set(balanced_v 3 "TRINE-Balanced-V")
add_trine_parameter_set(shortsig_i 4 "TRINE-ShortSig-I")
add_trine_parameter_set(shortsig_iii 5 "TRINE-ShortSig-III")
add_trine_parameter_set(shortsig_v 6 "TRINE-ShortSig-V")
