/*
 * inspector.h
 *
 *  Created on: Jul 24, 2018
 *      Author: newworld
 */

#ifndef LIBRARIES_PYTHON_SS_INSPECTOR_H_
#define LIBRARIES_PYTHON_SS_INSPECTOR_H_

#include <Python.h>
#include <frameobject.h>
#include <eosiolib_native/vm_api.h>

#ifdef __cplusplus
extern "C" {
#endif

void enable_injected_apis(int enabled);
int is_inspector_enabled(void);

void inspect_set_status(int n);

void whitelist_function(PyObject* func);
int inspect_function(PyObject* func);
int inspect_import_name(PyObject* name);

int inspect_setattr(PyObject* v, PyObject* name);
int inspect_getattr(PyObject* v, PyObject* name);
int inspect_opcode(int opcode);

int inspect_build_class(PyObject* cls);

void enable_create_code_object(int enable);
void enable_filter_set_attr(int enable);
void enable_filter_get_attr(int enable);
void enable_opcode_inspector(int enable);


void inspect_set_status(int status);
int inspect_get_status(void);

int is_create_code_object_enabled(void);

void add_code_object_to_current_account(PyCodeObject* co);
int is_current_code_object_in_current_account(void);



void memory_trace_start(void);
void memory_trace_stop(void);
int memory_trace_alloc(void* ptr, size_t size);
void memory_trace_realloc(void* old_ptr, void* new_ptr, size_t new_size);
void memory_trace_free(void* ptr);

int inspect_memory(void);
int inspect_obj_creation(PyTypeObject* type);
void enable_inspect_obj_creation(int enable);

int check_time(void);

int filter_attr(PyObject* v, PyObject* name);
void set_current_account(uint64_t account);

PyCodeObject *
PyCode_NewEx(int argcount, int kwonlyargcount,
           int nlocals, int stacksize, int flags,
           PyObject *code, PyObject *consts, PyObject *names,
           PyObject *varnames, PyObject *freevars, PyObject *cellvars,
           PyObject *filename, PyObject *name, int firstlineno,
           PyObject *lnotab);

int is_debug_mode(void);
void debug_print(const char* str, int len);

int inspector_enabled(void);


#ifdef __Pyx_PyCode_New
#undef __Pyx_PyCode_New
#endif

#define __Pyx_PyCode_New(a, k, l, s, f, code, c, n, v, fv, cell, fn, name, fline, lnos)\
        PyCode_NewEx(a, k, l, s, f, code, c, n, v, fv, cell, fn, name, fline, lnos)


#ifdef __cplusplus
}
#endif

#endif /* LIBRARIES_PYTHON_SS_INSPECTOR_H_ */
