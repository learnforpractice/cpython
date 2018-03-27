include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(BUILD_FLAGS -j${N})
endif()

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


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "build python3.6.4"
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
)

add_library( python3
             SHARED
            ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
)

if (APPLE)
    set(PYTHON_LIB "${CMAKE_SOURCE_DIR}/libraries/python/libpython3.6m.dylib")
else()
    set(PYTHON_LIB "${CMAKE_SOURCE_DIR}/libraries/python/libpython3.6m.so")
endif()

if (APPLE)
    target_link_libraries( python3 PUBLIC  ${PYTHON_LIB})
else()
    target_link_libraries( python3 PUBLIC ${PYTHON_LIB})
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

