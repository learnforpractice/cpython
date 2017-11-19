file(GLOB HEADERS "include/eos/chain/*.hpp")

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
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Makefile ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)


add_custom_target(dist
                   COMMAND [ -a ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib/libpython3.6m.a ] || make install
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   COMMENT "install custom python!"
#                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m.a
                   )

## SORT .cpp by most likely to change / break compile
add_library( tinypy
             SHARED
             tinypy.cpp
             ${HEADERS}
           )

add_library( ssl SHARED IMPORTED )
set_property( TARGET ssl PROPERTY IMPORTED_LOCATION /usr/local/Cellar/openssl/1.0.2l/lib/libssl.dylib )

add_library( crypto SHARED IMPORTED )
set_property( TARGET crypto PROPERTY IMPORTED_LOCATION /usr/local/Cellar/openssl/1.0.2l/lib/libcrypto.dylib )

add_library(tinypy3.6m  STATIC IMPORTED)
set_property(TARGET tinypy3.6m PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m.a)

target_link_libraries( tinypy ssl crypto intl tinypy3.6m)

target_include_directories( tinypy
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include
                            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                            )

INSTALL( TARGETS
   tinypy

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
