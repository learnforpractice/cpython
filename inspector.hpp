/*
 * inspector.hpp
 *
 *  Created on: Jul 24, 2018
 *      Author: newworld
 */

#ifndef LIBRARIES_VM_VM_CPYTHON_SS_INSPECTOR_HPP_
#define LIBRARIES_VM_VM_CPYTHON_SS_INSPECTOR_HPP_


#include "inspector.h"

#include <stdio.h>
#include <stdint.h>
#include <Python.h>

#include <map>
#include <vector>
#include <memory>
#include <string>


using namespace std;

class account_info {
public:
   account_info() {
      total_used_memory = 0;
      account = 0;
   }

   map<PyObject*, PyObject*> account_functions;
   map<PyCodeObject*, int> code_objects;
   map<PyObject*, int> class_objects;

   int total_used_memory;
   uint64_t account;
};

struct account_memory_info {
   std::shared_ptr<account_info> info;
   size_t size;
};

class inspector {
public:
   inspector();
   static inspector& get();

   void set_current_account(uint64_t account);

   int inspect_function(PyObject* func);
   int whitelist_function(PyObject* func);

   int whitelist_import_name(const char* name);
   int inspect_import_name(const char* name);

   int whitelist_attr(const char* name);
   int inspect_setattr(PyObject* v, PyObject* name);

   int inspect_getattr(PyObject* v, PyObject* name);

   int inspect_build_class(PyObject* cls);
   int is_class_in_current_account(PyObject* obj);

   int add_account_function(uint64_t account, PyObject* func);

   void add_code_object_to_current_account(PyCodeObject* co);
   int is_code_object_in_current_account(PyCodeObject* co);

   void enable_create_code_object_(int enable);

   void set_current_module(PyObject* mod);

   void memory_trace_start();
   void memory_trace_stop();
   void memory_trace_alloc(void* ptr, size_t new_size);
   void memory_trace_realloc(void* old_ptr, void* new_ptr, size_t new_size);
   void memory_trace_free(void* ptr);

   int inspect_obj_creation(PyTypeObject* type);
   int add_type_to_whitelist(PyTypeObject* type);

   int inspect_memory();

   int enabled;
   int status;
   int enable_create_code_object;
   int enable_filter_set_attr;
   int enable_filter_get_attr;
   int enable_inspect_obj_creation;

private:
   PyObject* current_module;
   uint64_t current_account;
   map<string, int> import_name_whitelist;
   map<int, const char*> opcode_map;

   map<PyObject*, PyObject*> function_whitelist;
   vector<int> opcode_blacklist;//158
   map<uint64_t, std::shared_ptr<account_info>> accounts_info_map;
   map<void*, std::unique_ptr<account_memory_info>> memory_info_map;
   map<void*, int> malloc_map;
   uint64_t total_used_memory;

   map<PyTypeObject*, int> type_whitelist_map;
};

//account_functions

#endif /* LIBRARIES_VM_VM_CPYTHON_SS_INSPECTOR_HPP_ */
