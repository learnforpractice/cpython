#include "injector.h"

static struct python_injected_apis s_api= {};

struct python_injected_apis* get_injected_apis() {
   return &s_api;
}

void add_function_to_whitelist(PyObject* func) {
   s_api.add_function_to_whitelist(func);
}

int is_function_in_whitelist(PyObject* func) {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.is_function_in_whitelist(func);
}

int is_import_name_in_whitelist(PyObject* name) {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.is_import_name_in_whitelist(name);
}

int check_time() {
   if (!s_api.enabled) {
      return 1;
   }
   return s_api.check_time();
}

