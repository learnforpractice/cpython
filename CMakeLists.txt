
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    COMMAND make
    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "build micropython python"
)

add_library( python
             STATIC
             ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
           )

target_link_libraries( python PUBLIC dl util)

