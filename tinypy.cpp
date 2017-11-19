/*
 * tinypython.cpp
 *
 *  Created on: Nov 18, 2017
 *      Author: newworld
 */

#include <Python.h>
#include <stdio.h>

extern "C" int init_tinypy() {
//   Py_Initialize();
   _Py_InitializeEx_Private(1, 0);
   Py_NoSiteFlag = 1;
   printf("hello,world\n");
//   PyRun_SimpleString("print('hello,world\\n')");
   PyRun_SimpleString("1+1");
//   PyRun_SimpleString("import sys");
//   Py_Finalize();
   return 0;
}
