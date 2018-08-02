#include "Python.h"
#include "hashtable.h"
#include "frameobject.h"
#include "pythread.h"
#include "osdefs.h"

#include "injector.h"

/* Trace memory blocks allocated by PyMem_RawMalloc() */
#define TRACE_RAW_MALLOC

/* Forward declaration */
static void tracemalloc_stop(void);

#ifdef Py_DEBUG
#  define TRACE_DEBUG
#endif

/* Protected by the GIL */
static struct {
    PyMemAllocatorEx mem;
    PyMemAllocatorEx raw;
    PyMemAllocatorEx obj;
} allocators;

static struct {
    /* Module initialized?
       Variable protected by the GIL */
    enum {
        TRACEMALLOC_NOT_INITIALIZED,
        TRACEMALLOC_INITIALIZED,
        TRACEMALLOC_FINALIZED
    } initialized;

    /* Is tracemalloc tracing memory allocations?
       Variable protected by the GIL */
    int tracing;

    /* limit of the number of frames in a traceback, 1 by default.
       Variable protected by the GIL. */
    int max_nframe;

    /* use domain in trace key?
       Variable protected by the GIL. */
    int use_domain;

    /* max alloc memory. */
    int max_malloc_size;

    /* max execution time in microsecond. */
    int max_execution_time;

} tracemalloc_config = {TRACEMALLOC_NOT_INITIALIZED, 0, 1, 0, 1024*1024 ,5000*1000};

#if defined(TRACE_RAW_MALLOC) && defined(WITH_THREAD)
/* This lock is needed because tracemalloc_free() is called without
   the GIL held from PyMem_RawFree(). It cannot acquire the lock because it
   would introduce a deadlock in PyThreadState_DeleteCurrent(). */
static PyThread_type_lock tables_lock;
#  define TABLES_LOCK() PyThread_acquire_lock(tables_lock, 1)
#  define TABLES_UNLOCK() PyThread_release_lock(tables_lock)
#else
   /* variables are protected by the GIL */
#  define TABLES_LOCK()
#  define TABLES_UNLOCK()
#endif


/* Size in bytes of currently traced memory.
   Protected by TABLES_LOCK(). */
static size_t tracemalloc_traced_memory = 0;

static int tracemalloc_is_out_off_memory = 0;


#if defined(WITH_THREAD) && defined(TRACE_RAW_MALLOC)
#define REENTRANT_THREADLOCAL

/* If your OS does not provide native thread local storage, you can implement
   it manually using a lock. Functions of thread.c cannot be used because
   they use PyMem_RawMalloc() which leads to a reentrant call. */
#if !(defined(_POSIX_THREADS) || defined(NT_THREADS))
#  error "need native thread local storage (TLS)"
#endif

static int tracemalloc_reentrant_key = -1;

/* Any non-NULL pointer can be used */
#define REENTRANT Py_True

static int
get_reentrant(void)
{
    void *ptr;

    assert(tracemalloc_reentrant_key != -1);
    ptr = PyThread_get_key_value(tracemalloc_reentrant_key);
    if (ptr != NULL) {
        assert(ptr == REENTRANT);
        return 1;
    }
    else
        return 0;
}

static void
set_reentrant(int reentrant)
{
    assert(reentrant == 0 || reentrant == 1);
    assert(tracemalloc_reentrant_key != -1);

    if (reentrant) {
        assert(!get_reentrant());
        PyThread_set_key_value(tracemalloc_reentrant_key, REENTRANT);
    }
    else {
        assert(get_reentrant());
        PyThread_set_key_value(tracemalloc_reentrant_key, NULL);
    }
}

#else

/* WITH_THREAD not defined: Python compiled without threads,
   or TRACE_RAW_MALLOC not defined: variable protected by the GIL */
static int tracemalloc_reentrant = 0;

static int
get_reentrant(void)
{
    return tracemalloc_reentrant;
}

static void
set_reentrant(int reentrant)
{
    assert(reentrant != tracemalloc_reentrant);
    tracemalloc_reentrant = reentrant;
}
#endif

static int notice = 0;

static int memory_run_out(void) {
   if (!inspect_memory()) {
      if (!notice) {
         notice = 1;
         return 1;
      }
   } else {
      notice = 0;
   }
   return 0;
}

static long long start_time = 0;

long long get_milliseconds() {
   struct timeval  tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000000LL + tv.tv_usec * 1LL ;
}

static int time_out(void) {
   struct timeval  tv;
   gettimeofday(&tv, NULL);
   long long time_now = tv.tv_sec * 1000000LL + tv.tv_usec ;

   if (!start_time) {
      return 0;
   }

   if (time_now - start_time >= tracemalloc_config.max_execution_time ) {
      start_time = 0;
      return 1;
   }
   return 0;
}


static int exit_eval_frame_check(void) {
   if (time_out()) {
      printf("++++++++++time out! exit_eval_frame\n");
      PyErr_Format(PyExc_RuntimeError, "time out!!!");
      return 1;
   }
   if (memory_run_out()) {
      printf("++++++++++memory out! exit_eval_frame\n");
      PyErr_Format(PyExc_MemoryError, "memory run out!!!");
      return 1;
   }
   return 0;
}

typedef int (*fn_check)(void);

static fn_check old_check = NULL;

extern fn_check set_exit_eval_frame_check(fn_check func);


static void*
tracemalloc_alloc(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    void *ptr;

    assert(elsize == 0 || nelem <= SIZE_MAX / elsize);

    if (nelem * elsize > 1024*500) {
       printf("large memory malloc detect :%ld\n", nelem * elsize);
       return NULL;
    }

    if (tracemalloc_traced_memory + nelem * elsize >= tracemalloc_config.max_malloc_size) {
       tracemalloc_is_out_off_memory = 1;
    }

    if (use_calloc)
        ptr = alloc->calloc(alloc->ctx, nelem, elsize);
    else
        ptr = alloc->malloc(alloc->ctx, nelem * elsize);
    if (ptr == NULL)
        return NULL;

    memory_trace_alloc(ptr, nelem*elsize);
    return ptr;
}

static void*
tracemalloc_realloc(void *ctx, void *ptr, size_t new_size)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    void *ptr2;

    ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);
    if (ptr2 == NULL)
        return NULL;

    if (ptr != NULL) {
        /* an existing memory block has been resized */
       memory_trace_realloc(ptr, ptr2, new_size);
    }
    else {
        /* new allocation */
       memory_trace_alloc(ptr2, new_size);
    }
    return ptr2;
}


static void
tracemalloc_free(void *ctx, void *ptr)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

    if (ptr == NULL)
        return;

     /* GIL cannot be locked in PyMem_RawFree() because it would introduce
        a deadlock in PyThreadState_DeleteCurrent(). */

    alloc->free(alloc->ctx, ptr);

    memory_trace_free(ptr);
}


static void*
tracemalloc_alloc_gil(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
    void *ptr;

    if (get_reentrant()) {
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
        if (use_calloc)
            return alloc->calloc(alloc->ctx, nelem, elsize);
        else
            return alloc->malloc(alloc->ctx, nelem * elsize);
    }

    /* Ignore reentrant call. PyObjet_Malloc() calls PyMem_Malloc() for
       allocations larger than 512 bytes, don't trace the same memory
       allocation twice. */
    set_reentrant(1);

    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);

    set_reentrant(0);
    return ptr;
}


static void*
tracemalloc_malloc_gil(void *ctx, size_t size)
{
    return tracemalloc_alloc_gil(0, ctx, 1, size);
}


static void*
tracemalloc_calloc_gil(void *ctx, size_t nelem, size_t elsize)
{
    return tracemalloc_alloc_gil(1, ctx, nelem, elsize);
}


static void*
tracemalloc_realloc_gil(void *ctx, void *ptr, size_t new_size)
{
    void *ptr2;

    if (get_reentrant()) {
        /* Reentrant call to PyMem_Realloc() and PyMem_RawRealloc().
           Example: PyMem_RawRealloc() is called internally by pymalloc
           (_PyObject_Malloc() and  _PyObject_Realloc()) to allocate a new
           arena (new_arena()). */
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

        ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);
        return ptr2;
    }

    /* Ignore reentrant call. PyObjet_Realloc() calls PyMem_Realloc() for
       allocations larger than 512 bytes. Don't trace the same memory
       allocation twice. */
    set_reentrant(1);

    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);

    set_reentrant(0);
    return ptr2;
}


#ifdef TRACE_RAW_MALLOC
static void*
tracemalloc_raw_alloc(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
#ifdef WITH_THREAD
    PyGILState_STATE gil_state;
#endif
    void *ptr;

    if (get_reentrant()) {
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
        if (use_calloc)
            return alloc->calloc(alloc->ctx, nelem, elsize);
        else
            return alloc->malloc(alloc->ctx, nelem * elsize);
    }

    /* Ignore reentrant call. PyGILState_Ensure() may call PyMem_RawMalloc()
       indirectly which would call PyGILState_Ensure() if reentrant are not
       disabled. */
    set_reentrant(1);

#ifdef WITH_THREAD
    gil_state = PyGILState_Ensure();
    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);
    PyGILState_Release(gil_state);
#else
    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);
#endif

    set_reentrant(0);
    return ptr;
}


static void*
tracemalloc_raw_malloc(void *ctx, size_t size)
{
    return tracemalloc_raw_alloc(0, ctx, 1, size);
}


static void*
tracemalloc_raw_calloc(void *ctx, size_t nelem, size_t elsize)
{
    return tracemalloc_raw_alloc(1, ctx, nelem, elsize);
}


static void*
tracemalloc_raw_realloc(void *ctx, void *ptr, size_t new_size)
{
#ifdef WITH_THREAD
    PyGILState_STATE gil_state;
#endif
    void *ptr2;

    if (get_reentrant()) {
        /* Reentrant call to PyMem_RawRealloc(). */
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

        ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);

        memory_trace_realloc(ptr, ptr2, new_size);
        return ptr2;
    }

    /* Ignore reentrant call. PyGILState_Ensure() may call PyMem_RawMalloc()
       indirectly which would call PyGILState_Ensure() if reentrant calls are
       not disabled. */
    set_reentrant(1);

#ifdef WITH_THREAD
    gil_state = PyGILState_Ensure();
    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);
    PyGILState_Release(gil_state);
#else
    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);
#endif

    set_reentrant(0);
    return ptr2;
}
#endif   /* TRACE_RAW_MALLOC */

static int
tracemalloc_start(int max_nframe)
{
    PyMemAllocatorEx alloc;
    size_t size;

    if (tracemalloc_config.tracing) {
        /* hook already installed: do nothing */
        return 0;
    }

    tracemalloc_is_out_off_memory = 0;


#ifdef TRACE_RAW_MALLOC
    alloc.malloc = tracemalloc_raw_malloc;
    alloc.calloc = tracemalloc_raw_calloc;
    alloc.realloc = tracemalloc_raw_realloc;
    alloc.free = tracemalloc_free;

    alloc.ctx = &allocators.raw;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);
#endif

    alloc.malloc = tracemalloc_malloc_gil;
    alloc.calloc = tracemalloc_calloc_gil;
    alloc.realloc = tracemalloc_realloc_gil;
    alloc.free = tracemalloc_free;

    alloc.ctx = &allocators.mem;
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &allocators.mem);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);

    alloc.ctx = &allocators.obj;
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &allocators.obj);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);

    /* everything is ready: start tracing Python memory allocations */
    tracemalloc_config.tracing = 1;

    old_check = set_exit_eval_frame_check((void*)exit_eval_frame_check);

    return 0;
}


static void
tracemalloc_stop(void)
{
    if (!tracemalloc_config.tracing)
        return;

    /* stop tracing Python memory allocations */
    tracemalloc_config.tracing = 0;

    /* unregister the hook on memory allocators */
#ifdef TRACE_RAW_MALLOC
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);
#endif
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &allocators.mem);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocators.obj);

    start_time = 0;
    set_exit_eval_frame_check(old_check);

}

PyDoc_STRVAR(tracemalloc_is_tracing_doc,
    "is_tracing()->bool\n"
    "\n"
    "True if the tracemalloc module is tracing Python memory allocations,\n"
    "False otherwise.");


static PyObject*
py_tracemalloc_is_tracing(PyObject *self)
{
    return PyBool_FromLong(tracemalloc_config.tracing);
}

PyDoc_STRVAR(tracemalloc_start_doc,
    "start(nframe: int=1)\n"
    "\n"
    "Start tracing Python memory allocations. Set also the maximum number \n"
    "of frames stored in the traceback of a trace to nframe.");

static PyObject*
py_tracemalloc_start(PyObject *self, PyObject *args)
{
    Py_ssize_t nframe = 1;
    int nframe_int = 1;

    if (!PyArg_ParseTuple(args, "|n:start", &nframe))
        return NULL;

    if (tracemalloc_start(nframe_int) < 0)
        return NULL;

    start_time = get_milliseconds();

    Py_RETURN_NONE;
}

PyDoc_STRVAR(tracemalloc_stop_doc,
    "stop()\n"
    "\n"
    "Stop tracing Python memory allocations and clear traces\n"
    "of memory blocks allocated by Python.");


static PyObject*
py_tracemalloc_stop(PyObject *self)
{
    tracemalloc_stop();
    Py_RETURN_NONE;
}


static PyMethodDef module_methods[] = {
    {"is_tracing", (PyCFunction)py_tracemalloc_is_tracing,
     METH_NOARGS, tracemalloc_is_tracing_doc},
    {"start", (PyCFunction)py_tracemalloc_start,
      METH_VARARGS, tracemalloc_start_doc},
    {"stop", (PyCFunction)py_tracemalloc_stop,
      METH_NOARGS, tracemalloc_stop_doc},
    /* sentinel */
    {NULL, NULL}
};

PyDoc_STRVAR(module_doc,
"Debug module to trace memory blocks allocated by Python.");

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "_tracemalloc",
    module_doc,
    0, /* non-negative size to be able to unload the module */
    module_methods,
    NULL,
};

PyMODINIT_FUNC
PyInit__tracemalloc(void)
{
    PyObject *m;
    m = PyModule_Create(&module_def);
    if (m == NULL)
        return NULL;

    return m;
}

int
_PyTraceMalloc_Init(void) {
   printf("+++_PyTraceMalloc_Init\n");
   return 1;
}

void
_PyTraceMalloc_Fini(void)
{
    printf("+++_PyTraceMalloc_Fini\n");
}

void
_PyMem_DumpTraceback(int fd, const void *ptr) {

}
