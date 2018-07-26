#include "injector.h"

static struct python_injected_apis s_api= {};

struct python_injected_apis* get_injected_apis() {
   return &s_api;
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
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.inspect_setattr(v, name);
}

int inspect_getattr(PyObject* v, PyObject* name) {
   if (!s_api.enabled) {
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

int check_time() {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.check_time();
}

