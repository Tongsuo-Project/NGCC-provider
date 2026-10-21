aux_source_directory(. KAT_SRC)

add_executable(${KAT_KEM_NAME} ${KAT_SRC})
target_link_libraries(${KAT_KEM_NAME} ${LIB_NAME})
