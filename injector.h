/*
 * injector.h
 *
 *  Created on: Jul 24, 2018
 *      Author: newworld
 */

#ifndef LIBRARIES_PYTHON_SS_INJECTOR_H_
#define LIBRARIES_PYTHON_SS_INJECTOR_H_

#include <Python.h>
#include <frameobject.h>

#ifdef __cplusplus
extern "C" {
#endif

PyFrameObject* PyEval_GetCurrentFrame();

struct python_injected_apis {
   int enabled;
   int enable_opcode_inspect;
   int enable_create_code_object;
   int status;

   void (*whitelist_function)(PyObject* func);
   int (*inspect_function)(PyObject* func);

   int (*whitelist_import_name)(const char* name);
   int (*inspect_import_name)(PyObject* name);

   int (*whitelist_attr)(const char* name);
   int (*inspect_setattr)(PyObject* v, PyObject* name);

   int (*inspect_getattr)(PyObject* v, PyObject* name);

   void (*add_code_object_to_current_account)(PyCodeObject* co);
   int (*is_code_object_in_current_account)(PyCodeObject* co);

   int (*inspect_opcode)(int opcode);

   void (*memory_trace_start)();
   void (*memory_trace_stop)();

   void (*memory_trace_alloc)(void* ptr, size_t new_size);
   void (*memory_trace_realloc)(void* old_ptr, void* new_ptr, size_t new_size);
   void (*memory_trace_free)(void* ptr);

   int (*check_time)();
};

struct python_injected_apis* get_injected_apis();

void inspect_set_status(int n);

void whitelist_function(PyObject* func);
int inspect_function(PyObject* func);
int inspect_import_name(PyObject* name);

int inspect_setattr(PyObject* v, PyObject* name);
int inspect_getattr(PyObject* v, PyObject* name);
int inspect_opcode(int opcode);

void enable_opcode_inspector(int enable);

void inspect_set_status(int status);
int inspect_get_status();

void enable_create_code_object(int enable);
int is_create_code_object_enabled();

void add_code_object_to_current_account(PyCodeObject* co);
int is_current_code_object_in_current_account();



void memory_trace_start();
void memory_trace_stop();
void memory_trace_alloc(void* ptr, size_t size);
void memory_trace_realloc(void* old_ptr, void* new_ptr, size_t new_size);
void memory_trace_free(void* ptr);

int check_time();

#ifdef __cplusplus
}
#endif

#endif /* LIBRARIES_PYTHON_SS_INJECTOR_H_ */
