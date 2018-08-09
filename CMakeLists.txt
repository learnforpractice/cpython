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

#./configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/dist --without-threads


add_library( python3-ss
          STATIC
         Modules/_struct.c

         injector.c
         Modules/config.c
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
         Modules/_tracemalloc.c
         Modules/_weakref.c
         Modules/atexitmodule.c
         Modules/config.c
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
         Python/random.c
         Python/structmember.c
         Python/symtable.c
         Python/sysmodule.c
         Python/thread.c
         Python/traceback.c

         softfloat/softfloat.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/eos/wrap/pydebug.wrap.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/pyconfig.h
)

#-DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes    -std=c99 -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers   -I. -I./Include    -DPy_BUILD_CORE -o Objects/abstract.o Objects/abstract.c

target_compile_options(python3-ss PRIVATE -DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes    -std=c99 -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers  -DPy_BUILD_CORE)

if (APPLE)
    target_link_libraries( python3-ss PUBLIC ${OPENSSL_LIBRARIES} intl softfloat)
else()
    target_link_libraries( python3-ss PUBLIC ${OPENSSL_LIBRARIES} softfloat)
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

