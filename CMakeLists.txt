include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(BUILD_FLAGS -j${N})
endif()

set(PYTHON_LIB "${CMAKE_CURRENT_SOURCE_DIR}/libpython3.7m${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(ENABLE_SHARED "")


set( ENV{OPENSSL_ROOT} "${OPENSSL_ROOT}" )

if(NOT APPLE)
     # Linux or other unix
    set( ENV{LD_LIBRARY_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/dist/lib" )
endif(NOT APPLE)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
#    COMMAND python pre_config.py "${OPENSSL_ROOT}"
    COMMAND bash configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist CFLAGS="-D_POSIX_THREADS" --with-openssl=${OPENSSL_ROOT}  --enable-shared
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "configure python3.7"
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configure ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h.in
)

add_custom_command(
    OUTPUT ${PYTHON_LIB}
    COMMAND OPENSSL_ROOT=${OPENSSL_ROOT};make ${BUILD_FLAGS}
    COMMAND make install
    COMMAND LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/dist/bin/python3 -m pip install cython
    COMMAND LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/dist/bin/python3 -m pip install ptvsd==3.0.0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "making python3.7"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

add_library( python3
            ${CMAKE_CURRENT_SOURCE_DIR}/Programs/python.c
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



set(PYTHON3 "LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/dist/bin/python3")
 
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/eos/wrap/pydebug.wrap.cpp
    COMMAND ${PYTHON3} -m cython --cplus ${CMAKE_CURRENT_SOURCE_DIR}/eos/cython/pydebug.pyx -o ${CMAKE_CURRENT_SOURCE_DIR}/eos/wrap/pydebug.wrap.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/eos/cython/pydebug.pyx
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/inspector/inspector.wrap.cpp
    COMMAND ${PYTHON3} -m cython --cplus ${CMAKE_CURRENT_SOURCE_DIR}/inspector/inspector.pyx -o ${CMAKE_CURRENT_SOURCE_DIR}/inspector/inspector.wrap.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/inspector/inspector.pyx
)


#./configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist --without-threads


add_library( python3-ss
          STATIC
         Modules/_struct.c

         inspector/inspector.cpp
         inspector/inspector.wrap.cpp

         Modules/config.c
         Modules/_threadmodule.c
         Modules/_abc.c
         Modules/_codecsmodule.c
         Modules/_collectionsmodule.c
         Modules/_functoolsmodule.c

         Modules/_io/_iomodule.c
         Modules/_io/bufferedio.c
         Modules/_io/bytesio.c
         Modules/_io/fileio.c
         Modules/_io/iobase.c
         Modules/_io/stringio.c
         Modules/_io/textio.c

         Modules/_localemodule.c
         Modules/_math.c
         Modules/_operator.c
         Modules/_sre.c
#         Modules/_ssl.c
         Modules/_stat.c
#         Modules/_threadmodule.c
         Modules/_tracemalloc-ss.c
         Modules/_weakref.c
         Modules/atexitmodule.c
         Modules/errnomodule.c
         Modules/faulthandler.c
         Modules/gcmodule.c
         Modules/getbuildinfo.c
         Modules/getpath.c
         Modules/hashtable.c
         Modules/itertoolsmodule.c
         Modules/main.c
         Modules/posixmodule.c
         Modules/pwdmodule.c
         Modules/signalmodule.c
         Modules/symtablemodule.c
         Modules/timemodule.c
         Modules/xxsubtype.c
         Modules/zipimport.c

         Objects/call.c
         Objects/abstract.c
         Objects/accu.c
         Objects/boolobject.c
         Objects/bytearrayobject.c
         Objects/bytes_methods.c
         Objects/bytesobject.c
         Objects/capsule.c
         Objects/cellobject.c
         Objects/classobject.c
         Objects/codeobject.c
         Objects/complexobject.c
         Objects/descrobject.c
         Objects/dictobject.c
         Objects/enumobject.c
         Objects/exceptions.c
         Objects/fileobject.c
#         Objects/floatobject.c
         Objects/softfloatobject.c
         Objects/frameobject.c
         Objects/funcobject.c
         Objects/genobject.c
         Objects/iterobject.c
         Objects/listobject.c
         Objects/longobject.c
         Objects/memoryobject.c
         Objects/methodobject.c
         Objects/moduleobject.c
         Objects/namespaceobject.c
         Objects/object.c
         Objects/obmalloc.c
         Objects/odictobject.c
         Objects/rangeobject.c
         Objects/setobject.c
         Objects/sliceobject.c
         Objects/structseq.c
         Objects/tupleobject.c
         Objects/typeobject.c
         Objects/unicodectype.c
         Objects/unicodeobject.c
         Objects/weakrefobject.c

         Parser/acceler.c
         Parser/bitset.c
         Parser/firstsets.c
         Parser/grammar.c
         Parser/grammar1.c
         Parser/listnode.c
         Parser/metagrammar.c
         Parser/myreadline.c
         Parser/node.c
         Parser/parser.c
         Parser/parsetok.c
         Parser/pgen.c
         Parser/tokenizer.c

#         Programs/_freeze_importlib.c
#         Programs/_testembed.c
#         Programs/python.c

         Python/ast_unparse.c
         Python/bootstrap_hash.c
         Python/hamt.c
         Python/ast_opt.c
         Python/pathconfig.c
         Python/context.c
         Python/_warnings.c
         Python/asdl.c
         Python/ast.c
         Python/bltinmodule.c
         Python/ceval.c
         Python/codecs.c
         Python/compile.c
         Python/dtoa.c
         Python/dynamic_annotations.c
#         Python/dynload_shlib.c
         Python/errors.c
         Python/fileutils.c
         Python/formatter_unicode.c
         Python/frozen.c
         Python/frozenmain.c
         Python/future.c
         Python/getargs.c
         Python/getcompiler.c
         Python/getcopyright.c
         Python/getopt.c
         Python/getplatform.c
         Python/getversion.c
         Python/graminit.c
         Python/import.c
#         Python/importdl.c
         Python/marshal.c
         Python/modsupport.c
         Python/mysnprintf.c
         Python/mystrtoul.c
         Python/peephole.c
         Python/pyarena.c
         Python/pyctype.c
         Python/pyfpe.c
         Python/pyhash.c
         Python/pylifecycle.c
         Python/pymath.c
         Python/pystate.c
         Python/pystrcmp.c
         Python/pystrhex.c
         Python/pystrtod.c
         Python/Python-ast.c
         Python/pythonrun.c
         Python/pytime.c
#         Python/random.c
         Python/structmember.c
         Python/symtable.c
         Python/sysmodule.c
         Python/thread.c
         Python/traceback.c

         softfloat/softfloat.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

#-DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes    -std=c99 -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers   -I. -I./Include    -DPy_BUILD_CORE -o Objects/abstract.o Objects/abstract.c


set(PYTHON_FLAGS -DNDEBUG -DPYTHON_SS -DPy_BUILD_CORE -DPREFIX="" -DEXEC_PREFIX="" -DVERSION="3.7" -DVPATH="" -DPYTHONPATH="")

target_compile_options(python3-ss PRIVATE $<$<COMPILE_LANGUAGE:C>:${PYTHON_FLAGS} -g -msoft-float -m64 -fwrapv -O3 -Wall -Wstrict-prototypes -std=c99 -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers> )

if (APPLE)
    target_link_libraries( python3-ss PUBLIC ${OPENSSL_LIBRARIES} intl softfloat)
else()
    target_link_libraries( python3-ss PUBLIC ${OPENSSL_LIBRARIES} softfloat)
endif()

add_dependencies(python3-ss python3)

target_include_directories( python3-ss
                            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include
                            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                            PRIVATE ${CMAKE_SOURCE_DIR}/contracts
                            )

INSTALL( TARGETS
   python3-ss

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)