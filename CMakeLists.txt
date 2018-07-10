include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(BUILD_FLAGS -j${N})
endif()


if(FALSE)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h)
    execute_process(
        COMMAND bash configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist --enable-shared
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE rv)
    if(NOT rv EQUAL 0)
        message(FATAL_ERROR "error: build python failed")
    endif()
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dist)
    execute_process(
        COMMAND make ${BUILD_FLAGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE rv)
    if(NOT rv EQUAL 0)
        message(FATAL_ERROR "error: build python failed")
    endif()
    execute_process(
        COMMAND make install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE rv)

    if(NOT rv EQUAL 0)
        message(FATAL_ERROR "error: build python failed")
    endif()

endif()


if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dist)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib/python3.6/site-packages/cython.py)
        execute_process(
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/dist/bin/python3 -m pip install cython
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/lib
            RESULT_VARIABLE rv)
        if(NOT rv EQUAL 0)
            message(FATAL_ERROR "error: install cython failed")
        endif()
    endif()
endif()

endif()

set(PYTHON_LIB "${CMAKE_CURRENT_SOURCE_DIR}/libpython3.6m${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(PYTHON_LIB_INSTALL "${CMAKE_CURRENT_SOURCE_DIR}/dist/lib/libpython3.6m${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(ENABLE_SHARED "")


set(PYTHON3 "${CMAKE_SOURCE_DIR}/libraries/python/dist/bin/python3")
set( ENV{OPENSSL_ROOT} "${OPENSSL_ROOT}" )

if(NOT APPLE)
     # Linux or other unix
    set( ENV{LD_LIBRARY_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/dist/lib" )
endif(NOT APPLE)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
#    COMMAND python pre_config.py "${OPENSSL_ROOT}"
    COMMAND bash configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist --enable-shared
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "configure python3.6.4"
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configure
)

add_custom_command(
    OUTPUT ${PYTHON_LIB}
    COMMAND OPENSSL_ROOT=${OPENSSL_ROOT};make ${BUILD_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "making python3.6.4"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

add_custom_command(
    OUTPUT ${PYTHON_LIB_INSTALL}
    COMMAND make install
    COMMAND LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/dist/bin/python3 -m pip install cython
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "installing python3.6.4"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h ${PYTHON_LIB}
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "linking python3.6.4"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h ${PYTHON_LIB_INSTALL}
)

add_library( python3
	     ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
            ${CMAKE_CURRENT_SOURCE_DIR}/start.cpp 
            ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
            ${PYTHON_LIB}
)


if (APPLE)
    target_link_libraries( python3 PUBLIC intl ${PYTHON_LIB} ${OPENSSL_LIBRARIES} )
else()
    target_link_libraries( python3 PUBLIC ${PYTHON_LIB} ${OPENSSL_LIBRARIES} util  -Xlinker -export-dynamic)
endif()

target_include_directories( python3
                            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include
                            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                            )

INSTALL( TARGETS
   python3

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

