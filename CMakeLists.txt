add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Makefile
    COMMAND echo "./configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist"
    COMMAND ./configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "configure custom python"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configure ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Setup.dist
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m.a
    COMMAND make -j7
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "build custom python"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Makefile
)

add_custom_target(install ALL
                   COMMAND [ -a ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib/libpython3.6m.a ] || make install
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   COMMENT "install custom python!"
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m.a
                   )


