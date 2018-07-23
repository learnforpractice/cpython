include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(BUILD_FLAGS -j${N})
endif()

set(PYTHON3 "LD_LIBRARY_PATH=${CMAKE_SOURCE_DIR}/libraries/python;${CMAKE_SOURCE_DIR}/libraries/python/dist/bin/python3")
 
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/eos/wrap/pydebug.wrap.cpp
    COMMAND ${PYTHON3} -m cython --cplus ${CMAKE_CURRENT_SOURCE_DIR}/eos/cython/pydebug.pyx -o ${CMAKE_CURRENT_SOURCE_DIR}/eos/wrap/pydebug.wrap.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/eos/cython/pydebug.pyx
)

set(PYTHON_SS_LIB "${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m.a")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
    COMMAND python pre_config.py "${OPENSSL_ROOT}"
    COMMAND bash configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist --without-threads
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "configure python3.6.3-single-thread"
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configure
)

add_custom_command(
    OUTPUT ${PYTHON_SS_LIB}
    COMMAND make ${BUILD_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "make python3.6.3-single-thread"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "build python3.6.3-single-thread"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

add_library( python3-ss
             STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/eos/wrap/pydebug.wrap.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
            ${PYTHON_SS_LIB}
)


if (APPLE)
    target_link_libraries( python3-ss PUBLIC ${PYTHON_SS_LIB} ${OPENSSL_LIBRARIES} intl)
else()
    target_link_libraries( python3-ss PUBLIC ${PYTHON_SS_LIB} ${OPENSSL_LIBRARIES})
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

