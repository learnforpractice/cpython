/*
 * injector.h
 *
 *  Created on: Jul 24, 2018
 *      Author: newworld
 */

#ifndef LIBRARIES_PYTHON_SS_INJECTOR_H_
#define LIBRARIES_PYTHON_SS_INJECTOR_H_

#include <Python.h>

#ifdef __cplusplus
extern "C" {
#endif

struct python_injected_apis {
   int enabled;
   void (*add_function_to_whitelist)(PyObject* func);
   int (*is_function_in_whitelist)(PyObject* func);

   int (*add_import_name_to_whitelist)(const char* name);
   int (*is_import_name_in_whitelist)(PyObject* name);

   int (*is_opcode_in_whitelist)(int opcode);
   int (*check_time)();
};

struct python_injected_apis* get_injected_apis();

void add_function_to_whitelist(PyObject* func);
int is_function_in_whitelist(PyObject* func);
int is_import_name_in_whitelist(PyObject* name);

int check_time();

#ifdef __cplusplus
}
#endif

#endif /* LIBRARIES_PYTHON_SS_INJECTOR_H_ */
