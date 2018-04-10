#include <Python.h>
int gil_check_() {
   return PyGILState_Check();
}
