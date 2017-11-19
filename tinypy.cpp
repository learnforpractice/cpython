/*
 * tinypython.cpp
 *
 *  Created on: Nov 18, 2017
 *      Author: newworld
 */

#include <Python.h>
#include <stdio.h>

typedef void (*fn_init_modules)(void);

#define MAXPATHLEN 1024
static wchar_t env_home[MAXPATHLEN+1];

extern "C" int init_tinypy(fn_init_modules init_modules) {
   Py_NoSiteFlag = 1;
   const char *chome = "/Users/newworld/dev/pyeos/libraries/tinypy/dist";
   mbstowcs(env_home, chome, sizeof(env_home)/sizeof(env_home[0]));
   Py_SetPythonHome(env_home);
   Py_InitializeEx(0);
//   _Py_InitializeEx_Private(0, 1);
   PyEval_InitThreads();
   if (init_modules) {
      init_modules();
   }
   printf("hello,world\n");
   PyRun_SimpleString("import sys");
   PyRun_SimpleString("print(sys.path)");
   PyRun_SimpleString("from _heapq import *");
   PyRun_SimpleString("print('hello,world from python\\n')");
   PyRun_SimpleString("1+1");
   PyRun_SimpleString("import logging");
   PyRun_SimpleString("print('hello again')");
   PyRun_SimpleString("import tracemalloc;tracemalloc.set_max_malloc_size(2000*1024)");


//   Py_Finalize();
   return 0;
}
