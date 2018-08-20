#include "inspector.hpp"
#include <eosiolib_native/vm_api.h>
#include <opcode.h>
#include <pystate.h>

using namespace std;

//vm_cpython.pyx
string get_c_string(PyObject* s);
int py_inspect_getattr(PyObject* v, PyObject* name);
int py_inspect_setattr(PyObject* v, PyObject* name);
int py_inspect_function(PyObject* func);
int cy_is_class_in_current_account(PyObject* obj);

extern "C" {
   extern PyTypeObject PyStructType;
   extern PyTypeObject PyFileIO_Type;
   extern PyTypeObject PyBufferedReader_Type;
   extern PyTypeObject PyTextIOWrapper_Type;
}

static int noticed = 0;

static int memory_run_out(void) {
   if (!inspect_memory()) {
      if (!noticed) {
         noticed = 1;
         return 1;
      }
   } else {
      noticed = 0;
   }
   return 0;
}

static int exit_eval_frame_check(void) {
   static uint64_t counter = 0;
   if (!inspector::get().enabled) {
      return 0;
   }
   counter += 1;
   if (counter % 10 == 0) {
      if (!check_time()) {
         PyErr_Format(PyExc_RuntimeError, "time out!!!");
         return 1;
      }
   }
   if (memory_run_out()) {
      PyGC_Collect();
      memory_trace_stop();
      printf("++++++++++memory out! exit_eval_frame\n");
      PyErr_Format(PyExc_MemoryError, "memory run out!!!");
      return 1;
   }
   return 0;
}

typedef int (*fn_check)(void);

static fn_check old_check = NULL;

extern "C" fn_check set_exit_eval_frame_check(fn_check func);

int object_is_function(PyObject* func) {
   if (!func) {
      return 0;
   }
   PyTypeObject *tp = Py_TYPE(func);

//   printf(" ++++++++object_is_function: tp->tp_name %s\n", tp->tp_name);
   if (&PyMethodDescr_Type == Py_TYPE(func)) {
      return 1;
   }

   if (PyCFunction_Check(func)) {
      return 1;
   }
   if (PyFunction_Check(func)) {
      return 1;
   }
   if (PyMethod_Check(func)) {
      return 1;
   }
   return 0;
}

int filter_attr(PyObject* v, PyObject* name) {
    Py_ssize_t size;
    const char* cname;

    PyTypeObject *tp = Py_TYPE(v);

    if (!PyUnicode_Check(name)) {
        return 0;
    }

    if (tp->tp_getattro != NULL) {
       PyObject* o = (*tp->tp_getattro)(v, name);
       return object_is_function(o);
    }

    if (tp->tp_getattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL)
            return 0;

        PyObject* o = (*tp->tp_getattr)(v, (char*)name_str);
        return object_is_function(o);
    }
    return 0;
}


struct opcode_map
{
   int index;
   const char* name;
};

extern struct opcode_map _opcode_map[];

inspector::inspector() {
   enabled = 0;
   status = 0;
   enable_create_code_object = 0;
   enable_filter_set_attr = 0;
   enable_filter_get_attr = 0;
   enable_inspect_obj_creation = 0;

   total_used_memory = 0;
   current_module = nullptr;
   current_account = 0;
   opcode_blacklist.resize(158, 0);//158
//   opcode_blacklist[IMPORT_NAME] = 1;
//   opcode_blacklist[CALL_FUNCTION] = 1;
//   opcode_blacklist[CALL_FUNCTION_KW] = 1;
//   opcode_blacklist[CALL_FUNCTION_EX] = 1;

   old_check = set_exit_eval_frame_check(exit_eval_frame_check);

   import_name_whitelist["struct"] = 1;
   import_name_whitelist["_struct"] = 1;
   import_name_whitelist["db"] = 1;
   import_name_whitelist["eoslib"] = 1;
   for (int i=0;_opcode_map[i].index != 0;i++) {
      opcode_map[_opcode_map[i].index] = _opcode_map[i].name;
   }

   type_whitelist_map[&PyBaseObject_Type] = 1;
   type_whitelist_map[&PyType_Type] = 1;
//   type_whitelist_map[&_PyWeakref_RefType] = 1;
//   type_whitelist_map[&_PyWeakref_CallableProxyType] = 1;
//   type_whitelist_map[&_PyWeakref_ProxyType] = 1;
   type_whitelist_map[&PyLong_Type] = 1;
   type_whitelist_map[&PyBool_Type] = 1;
   type_whitelist_map[&PyByteArray_Type] = 1;
   type_whitelist_map[&PyBytes_Type] = 1;
   type_whitelist_map[&PyList_Type] = 1;
   type_whitelist_map[&_PyNone_Type] = 1;
   type_whitelist_map[&_PyNotImplemented_Type] = 1;
   type_whitelist_map[&PyTraceBack_Type] = 1;
   type_whitelist_map[&PySuper_Type] = 1;
   type_whitelist_map[&PyRange_Type] = 1;
   type_whitelist_map[&PyDict_Type] = 1;
   type_whitelist_map[&PyDictKeys_Type] = 1;
   type_whitelist_map[&PyDictValues_Type] = 1;
   type_whitelist_map[&PyDictItems_Type] = 1;
   type_whitelist_map[&PyODict_Type] = 1;
   type_whitelist_map[&PyODictKeys_Type] = 1;
   type_whitelist_map[&PyODictItems_Type] = 1;
   type_whitelist_map[&PyODictValues_Type] = 1;
   type_whitelist_map[&PyODictIter_Type] = 1;
   type_whitelist_map[&PySet_Type] = 1;
   type_whitelist_map[&PyUnicode_Type] = 1;
   type_whitelist_map[&PySlice_Type] = 1;
   type_whitelist_map[&PyStaticMethod_Type] = 1;
   type_whitelist_map[&PyComplex_Type] = 1;
   type_whitelist_map[&PyFloat_Type] = 1;
//   type_whitelist_map[&PyFrozenSet_Type] = 1;
   type_whitelist_map[&PyProperty_Type] = 1;
//   type_whitelist_map[&_PyManagedBuffer_Type] = 1;
   type_whitelist_map[&PyMemoryView_Type] = 1;
   type_whitelist_map[&PyTuple_Type] = 1;
   type_whitelist_map[&PyEnum_Type] = 1;
//   type_whitelist_map[&PyReversed_Type] = 1;
//   type_whitelist_map[&PyStdPrinter_Type] = 1;
//   type_whitelist_map[&PyCode_Type] = 1;
   type_whitelist_map[&PyFrame_Type] = 1;
   type_whitelist_map[&PyCFunction_Type] = 1;
   type_whitelist_map[&PyMethod_Type] = 1;
   type_whitelist_map[&PyFunction_Type] = 1;
//   type_whitelist_map[&PyDictProxy_Type] = 1;
//   type_whitelist_map[&PyGen_Type] = 1;
//   type_whitelist_map[&PyGetSetDescr_Type] = 1;
//   type_whitelist_map[&PyWrapperDescr_Type] = 1;
//   type_whitelist_map[&_PyMethodWrapper_Type] = 1;
//   type_whitelist_map[&PyEllipsis_Type] = 1;
//   type_whitelist_map[&PyMemberDescr_Type] = 1;
//   type_whitelist_map[&_PyNamespace_Type] = 1;
//   type_whitelist_map[&PyCapsule_Type] = 1;
//   type_whitelist_map[&PyLongRangeIter_Type] = 1;
//   type_whitelist_map[&PyCell_Type] = 1;
//   type_whitelist_map[&PyInstanceMethod_Type] = 1;
//   type_whitelist_map[&PyClassMethodDescr_Type] = 1;
//   type_whitelist_map[&PyMethodDescr_Type] = 1;
//   type_whitelist_map[&PyCallIter_Type] = 1;
//   type_whitelist_map[&PySeqIter_Type] = 1;
//   type_whitelist_map[&PyCoro_Type] = 1;
//   type_whitelist_map[&_PyCoroWrapper_Type] = 1;

   type_whitelist_map[(PyTypeObject*)PyExc_Exception] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_TypeError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_MemoryError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_AttributeError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_RecursionError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_OSError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_AssertionError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_ImportError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_OverflowError] = 1;

   type_whitelist_map[(PyTypeObject*)PyExc_NameError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_IndexError] = 1;
   type_whitelist_map[(PyTypeObject*)PyExc_BaseException] = 1;

//   type_whitelist_map[&PyFileIO_Type] = 1; //FIXME: dangerous type
   type_whitelist_map[&PyBufferedReader_Type] = 1;
   type_whitelist_map[&PyTextIOWrapper_Type] = 1;

   type_whitelist_map[&PyStructType] = 1;
}

inspector& inspector::get() {
   static inspector* inst = nullptr;
   if (!inst) {
      inst = new inspector();
   }
   return *inst;
}

int inspector::inspect_obj_creation(PyTypeObject* type) {
   if (!enable_inspect_obj_creation) {
      return 1;
   }

   if (type_whitelist_map.find(type) != type_whitelist_map.end()) {
      return 1;
   }

   if (is_class_in_current_account((PyObject*)type)) {
      return 1;
   }
   return 0;
}

int inspector::add_type_to_whitelist(PyTypeObject* type) {
   type_whitelist_map[type] = 1;
   return 1;
}

void inspector::set_current_account(uint64_t account) {
   current_account = account;

   auto it = accounts_info_map.find(account);
   if (it == accounts_info_map.end()) {
      auto _new_map = std::make_shared<account_info>();
      _new_map->account = account;
      accounts_info_map[account] = _new_map;
      it = accounts_info_map.find(account);
   }
}

int inspector::inspect_function(PyObject* func) {
   if (!enabled) {
      return 1;
   }
   //   printf("++++++inspector::inspect_function: %p\n", func);
   return py_inspect_function(func);
   return function_whitelist.find(func) != function_whitelist.end();
}

int inspector::whitelist_function(PyObject* func) {
   function_whitelist[func] = func;
   return 1;
}

int inspector::whitelist_import_name(const char* name) {
   import_name_whitelist[name] = 1;
   return 1;
}

int inspector::inspect_import_name(const char* name) {
   if (!enabled) {
      return 1;
   }
   return import_name_whitelist.find(name) != import_name_whitelist.end();
}

bool compare_string(Py_UNICODE * wstr, Py_ssize_t size, string& str) {
   if (size != str.size()) {
      return false;
   }

   for(int i=0;i<size;i++) {
      if (wstr[i] != str[i]) {
         return false;
      }
   }
   return true;
}

int inspector::inspect_setattr(PyObject* v, PyObject* name) {
   if (!enable_filter_set_attr) {
      return 1;
   }

   Py_ssize_t size;
   const char* _name = PyUnicode_AsUTF8AndSize(name, &size);
   if (!_name) {
      return 0;
   }

   if (size >= 2 && _name[0] == '_' && _name[1] == '_') {
      printf("inspect_setattr attribute accessing should not start with '_'\n");
      return 0;
   }

   if (current_module == v) {
      return 1;
   }

   PyObject* cls = (PyObject*)Py_TYPE(v);
   if (is_class_in_current_account(cls)) {
      return 1;
   }
   return 0;
}

int inspector::inspect_getattr(PyObject* v, PyObject* name) {
   if (!enable_filter_get_attr) {
      return 1;
   }

   Py_ssize_t size;
   const char *cname = PyUnicode_AsUTF8AndSize(name, &size);
   if (!cname) {
      return 0;
   }

   if (size >= 2) {
      if (cname[0] == '_' && cname[1] == '_') {
         if (strcmp(cname, "__init__") == 0) {
            return 1;
         }
         return 0;
      }
   }

   if (current_module == v) {
      return 1;
   }

   PyObject* cls = (PyObject*)Py_TYPE(v);
   if (is_class_in_current_account(cls)) {
      return 1;
   }
   return filter_attr(v, name);
}

int inspector::inspect_build_class(PyObject* cls) {
   if (!enabled) {
      return 1;
   }
   return 1;

   PyFrameObject *f = PyThreadState_GET()->frame;
   if (f != NULL) {
      auto it = accounts_info_map.find(current_account);
      if (it == accounts_info_map.end()) {
         printf("+++inspect_build_class account not found!");
         return 0;
      }

      auto& info = it->second;
      if (info->code_objects.find(f->f_code) != info->code_objects.end()) {
//         printf("-----add cls to info %p\n", cls);
         info->class_objects[cls] = 1;
         return 1;
      } else {
         printf("-----add cls to info %p, code object not in current account\n", cls);
      }
   }
   return 0;
}

int inspector::is_class_in_current_account(PyObject* cls) {
   return cy_is_class_in_current_account(cls);
}

int inspector::add_account_function(uint64_t account, PyObject* func) {
   auto it = accounts_info_map.find(account);
   if (it == accounts_info_map.end()) {
      auto _new_map = std::make_shared<account_info>();
      _new_map->account = current_account;
      accounts_info_map[account] = _new_map;
      it = accounts_info_map.find(account);
   }
   it->second->account_functions[func] = func;
   return 1;
}

void inspector::add_code_object_to_current_account(PyCodeObject* co) {
   if (!enable_create_code_object) {
      return;
   }
   auto it = accounts_info_map.find(current_account);
   it->second->code_objects[co] = 1;
}

int inspector::is_code_object_in_current_account(PyCodeObject* co) {
   if (!enabled) {
      return 1;
   }

   auto it = accounts_info_map.find(current_account);
   if (it == accounts_info_map.end()) {
      return 0;
   }

   auto& code_objs = it->second->code_objects;
   if (code_objs.find(co) == code_objs.end()) {
      return 0;
   }
   return 1;
}

void inspector::set_current_module(PyObject* mod) {
   current_module = mod;
}

void inspector::memory_trace_start() {

}

void inspector::memory_trace_stop() {
   malloc_map.clear();
   total_used_memory = 0;
}

void inspector::memory_trace_alloc(void* ptr, size_t new_size) {
   if (!enabled) {
      return;
   }
//   memory_info_map[ptr] = std::move(_memory_info);
//   printf("+++memory_trace_alloc:%p %d \n", ptr, new_size);
   malloc_map[ptr] = new_size;
   total_used_memory += new_size;
}

void inspector::memory_trace_realloc(void* old_ptr, void* new_ptr, size_t new_size) {
   if (!enabled) {
      return;
   }
//   printf("+++memory_trace_realloc: %p %p %d \n", old_ptr, new_ptr, new_size);
   auto it = malloc_map.find(old_ptr);
   if (it != malloc_map.end()) {
      total_used_memory -= it->second;
      malloc_map.erase(it);
   }

   malloc_map[new_ptr] = new_size;
   total_used_memory += new_size;
}

void inspector::memory_trace_free(void* ptr) {
   if (!enabled) {
      return;
   }

   auto it = malloc_map.find(ptr);
   if (it != malloc_map.end()) {
//      printf("+++memory_trace_free: %p %d\n", ptr, it->second);
      total_used_memory -= it->second;
      malloc_map.erase(it);
   }
}

int inspector::inspect_memory() {
   if (!enabled) {
      return 1;
   }

//   printf("+++total_used_memory %d \n", total_used_memory);
   if (total_used_memory >= 2000*1024) {
      printf("+++total_used_memory %d \n", total_used_memory);
   }
   return total_used_memory <= 2000*1024;
}

extern "C" {

void enable_injected_apis(int enabled) {
   inspector::get().enabled = enabled;
}

int is_inspector_enabled() {
   return inspector::get().enabled;
}

void whitelist_function(PyObject* func) {
   inspector::get().whitelist_function(func);
}

int inspect_function(PyObject* func) {
   return inspector::get().inspect_function(func);
}

int whitelist_import_name(const char* name) {
   return inspector::get().whitelist_import_name(name);
}

int inspect_import_name(PyObject* name) {
   Py_ssize_t size;
   const char* _name = PyUnicode_AsUTF8AndSize(name, &size);
   return inspector::get().inspect_import_name(_name);
}

int inspect_setattr(PyObject* v, PyObject* name) {
   return inspector::get().inspect_setattr(v, name);
}

int inspect_getattr(PyObject* v, PyObject* name) {
   return inspector::get().inspect_getattr(v, name);
}

int inspect_build_class(PyObject* cls) {
   return inspector::get().inspect_build_class(cls);
}

void enable_create_code_object(int enable) {
   if (enable) {
      inspector::get().enable_create_code_object = 1;
   } else {
      inspector::get().enable_create_code_object = -1;
   }
}

void enable_filter_set_attr(int enable) {
   if (enable) {
      inspector::get().enable_filter_set_attr = 1;
   } else {
      inspector::get().enable_filter_set_attr = 0;
   }
}

void enable_filter_get_attr(int enable) {
   if (enable) {
      inspector::get().enable_filter_get_attr = 1;
   } else {
      inspector::get().enable_filter_get_attr = 0;
   }
}


void add_code_object_to_current_account(PyCodeObject* co) {
   inspector::get().add_code_object_to_current_account(co);
}

int is_code_object_in_current_account(PyCodeObject* co) {
   return inspector::get().is_code_object_in_current_account(co);
}

int is_current_code_object_in_current_account() {
   if (!inspector::get().enabled) {
      return 1;
   }
   PyFrameObject *f = PyThreadState_GET()->frame;
   if (!f) {
      return 0;
   }
   if (!f->f_code) {
      return 0;
   }
   return is_code_object_in_current_account(f->f_code);
}


int is_create_code_object_enabled() {
   if (inspector::get().enable_create_code_object == 0) {
      return 1;
   }
   return inspector::get().enable_create_code_object == 1;
}

void memory_trace_start() {
   noticed = 0;
   inspector::get().memory_trace_start();
}

void memory_trace_stop() {
   inspector::get().memory_trace_stop();
}

void memory_trace_alloc(void* ptr, size_t size) {
   inspector::get().memory_trace_alloc(ptr, size);
}

void memory_trace_realloc(void* old_ptr, void* new_ptr, size_t new_size) {
   inspector::get().memory_trace_realloc(old_ptr, new_ptr, new_size);
}

void memory_trace_free(void* ptr) {
   inspector::get().memory_trace_free(ptr);
}

int inspect_memory() {
   return inspector::get().inspect_memory();
}

int inspect_obj_creation(PyTypeObject* type) {
   return inspector::get().inspect_obj_creation(type);
}

void enable_inspect_obj_creation(int enable) {
   if (enable) {
      inspector::get().enable_inspect_obj_creation = 1;
   } else {
      inspector::get().enable_inspect_obj_creation = 0;
   }
}

void set_current_account(uint64_t account) {
   inspector::get().set_current_account(account);
}

int check_time() {
   try {
      get_vm_api()->checktime();
   } catch (...) {
      return 0;
   }
   return 1;
}

}
