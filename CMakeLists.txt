set(PYTHON_LIB_SS "${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m.a")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
    COMMAND python pre_config.py "${OPENSSL_ROOT}"
    COMMAND bash configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist --without-threads
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "configure python3.6.3 single thread"
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configure
)

add_custom_command(
    OUTPUT ${PYTHON_LIB_SS}
    COMMAND make ${BUILD_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "make python3.6.3 single thread"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

add_library( python3-ss
             STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/tinypy.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
            ${PYTHON_LIB_SS}
)


if (APPLE)
    target_link_libraries( python3-ss PUBLIC ${PYTHON_LIB_SS})
else()
    target_link_libraries( python3-ss PUBLIC ${PYTHON_LIB_SS})
endif()

target_include_directories( python3-ss
                            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include
                            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                            )

INSTALL( TARGETS
   python3-ss

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

