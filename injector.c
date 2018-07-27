#include "injector.h"

static struct python_injected_apis s_api= {};

struct python_injected_apis* get_injected_apis() {
   return &s_api;
}

void enable_injected_apis(int enabled) {
   s_api.enabled = enabled;
}

void whitelist_function(PyObject* func) {
   s_api.whitelist_function(func);
}

int inspect_function(PyObject* func) {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.inspect_function(func);
}

int whitelist_import_name(const char* name) {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.whitelist_import_name(name);
}

int inspect_import_name(PyObject* name) {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.inspect_import_name(name);
}

int whitelist_attr_(const char* name) {
   if (!s_api.enabled) {
      return 1;
   }
   return 1;
}

int inspect_setattr(PyObject* v, PyObject* name) {
   if (!s_api.enable_filter_set_attr) {
      return 1;
   }
   return s_api.inspect_setattr(v, name);
}

int inspect_getattr(PyObject* v, PyObject* name) {
   if (!s_api.enable_filter_get_attr) {
      return 1;
   }
   return s_api.inspect_getattr(v, name);
}

int inspect_opcode(int opcode) {
   if (!s_api.enable_opcode_inspect) {
      return 1;
   }
   int ret = s_api.inspect_opcode(opcode);
   if (!ret) {
      printf("opcode %d not allowed\n", opcode);
   }
   return ret;
}

void enable_opcode_inspector(int enable) {
   if (enable) {
      s_api.enable_opcode_inspect = 1;
   } else {
      s_api.enable_opcode_inspect = 0;
   }
}

int inspect_get_status() {
   return s_api.status;
}

void enable_create_code_object(int enable) {
   if (enable) {
      s_api.enable_create_code_object = 1;
   } else {
      s_api.enable_create_code_object = -1;
   }
}

void enable_filter_set_attr(int enable) {
   if (enable) {
      s_api.enable_filter_set_attr = 1;
   } else {
      s_api.enable_filter_set_attr = 0;
   }
}

void enable_filter_get_attr(int enable) {
   if (enable) {
      s_api.enable_filter_get_attr = 1;
   } else {
      s_api.enable_filter_get_attr = 0;
   }
}


void add_code_object_to_current_account(PyCodeObject* co) {
   if (!s_api.enabled) {
      return;
   }
   s_api.add_code_object_to_current_account(co);
}

int is_code_object_in_current_account(PyCodeObject* co) {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.is_code_object_in_current_account(co);
}

int is_current_code_object_in_current_account() {
   if (!s_api.enabled) {
      return 1;
   }
   PyFrameObject* f = PyEval_GetCurrentFrame();
   if (!f) {
      return 0;
   }
   if (!f->f_code) {
      return 0;
   }
   return is_code_object_in_current_account(f->f_code);
}


int is_create_code_object_enabled() {
   if (s_api.enable_create_code_object == 0) {
      return 1;
   }
   return s_api.enable_create_code_object == 1;
}

void memory_trace_start() {
   if (!s_api.enabled) {
      return;
   }
   s_api.memory_trace_start();
}

void memory_trace_stop() {
   if (!s_api.enabled) {
      return;
   }
   s_api.memory_trace_stop();
}

void memory_trace_alloc(void* ptr, size_t size) {
   if (!s_api.enabled) {
      return;
   }
   s_api.memory_trace_alloc(ptr, size);
}

void memory_trace_realloc(void* old_ptr, void* new_ptr, size_t new_size) {
   if (!s_api.enabled) {
      return;
   }
   s_api.memory_trace_realloc(old_ptr, new_ptr, new_size);
}

void memory_trace_free(void* ptr) {
   if (!s_api.enabled) {
      return;
   }
   s_api.memory_trace_free(ptr);
}

int check_time() {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.check_time();
}

