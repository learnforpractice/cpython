#include "Python.h"
#include "hashtable.h"
#include "frameobject.h"
#include "pythread.h"
#include "osdefs.h"

#include "clinic/_tracemalloc.c.h"

#include "objimpl.h"
#include "injector.h"

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

static int exit_eval_frame_check(void) {
   if (!check_time()) {
      PyErr_Format(PyExc_RuntimeError, "time out!!!");
      return 1;
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

extern fn_check set_exit_eval_frame_check(fn_check func);


#define MAX_NFRAME 10

/*[clinic input]
module _tracemalloc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=708a98302fc46e5f]*/

/* Trace memory blocks allocated by PyMem_RawMalloc() */
//#define TRACE_RAW_MALLOC

/* Forward declaration */
static void tracemalloc_stop(void);
static void* raw_malloc(size_t size);
static void raw_free(void *ptr);

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

} tracemalloc_config = {TRACEMALLOC_NOT_INITIALIZED, 0, 1, 0, 0};


#define DEFAULT_DOMAIN 0

/* TRACE_RAW_MALLOC not defined: variable protected by the GIL */
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


static void*
raw_malloc(size_t size)
{
    return allocators.raw.malloc(allocators.raw.ctx, size);
}

static void
raw_free(void *ptr)
{
    allocators.raw.free(allocators.raw.ctx, ptr);
}

#undef SIZE_MAX
#define SIZE_MAX (10*1024*1024)

static void*
tracemalloc_alloc(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    void *ptr;

    if (elsize == 0 || nelem > SIZE_MAX / elsize) {
       return NULL;
    }

    if (use_calloc)
        ptr = alloc->calloc(alloc->ctx, nelem, elsize);
    else
        ptr = alloc->malloc(alloc->ctx, nelem * elsize);
    if (ptr == NULL)
        return NULL;

    memory_trace_alloc(ptr, nelem * elsize);
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
       memory_trace_realloc(ptr, ptr2, new_size);
    }
    else {
        /* new allocation */
        memory_trace_alloc(ptr, new_size);

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
        if (ptr2 != NULL && ptr != NULL) {
           memory_trace_free(ptr);
//            TABLES_LOCK();
//            REMOVE_TRACE(ptr);
//            TABLES_UNLOCK();
        }
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

static void*
tracemalloc_raw_alloc(int use_calloc, void *ctx, size_t nelem, size_t elsize)
{
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
    ptr = tracemalloc_alloc(use_calloc, ctx, nelem, elsize);
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
    PyGILState_STATE gil_state;
    void *ptr2;

    if (get_reentrant()) {
        /* Reentrant call to PyMem_RawRealloc(). */
        PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;

        ptr2 = alloc->realloc(alloc->ctx, ptr, new_size);

        if (ptr2 != NULL && ptr != NULL) {
           memory_trace_free(ptr);
        }
        return ptr2;
    }

    /* Ignore reentrant call. PyGILState_Ensure() may call PyMem_RawMalloc()
       indirectly which would call PyGILState_Ensure() if reentrant calls are
       not disabled. */
    set_reentrant(1);
    ptr2 = tracemalloc_realloc(ctx, ptr, new_size);
    set_reentrant(0);
    return ptr2;
}


static int
tracemalloc_init(void)
{
    if (tracemalloc_config.initialized == TRACEMALLOC_FINALIZED) {
        PyErr_SetString(PyExc_RuntimeError,
                        "the tracemalloc module has been unloaded");
        return -1;
    }

    if (tracemalloc_config.initialized == TRACEMALLOC_INITIALIZED)
        return 0;

    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);


    tracemalloc_config.initialized = TRACEMALLOC_INITIALIZED;
    return 0;
}


static void
tracemalloc_deinit(void)
{
    if (tracemalloc_config.initialized != TRACEMALLOC_INITIALIZED)
        return;
    tracemalloc_config.initialized = TRACEMALLOC_FINALIZED;

    tracemalloc_stop();
}


static int
tracemalloc_start(int max_nframe)
{
    PyMemAllocatorEx alloc;
    size_t size;

    if (max_nframe < 1 || max_nframe > MAX_NFRAME) {
        PyErr_Format(PyExc_ValueError,
                     "the number of frames must be in range [1; %i]",
                     (int)MAX_NFRAME);
        return -1;
    }

    if (tracemalloc_init() < 0) {
        return -1;
    }

    if (tracemalloc_config.tracing) {
        /* hook already installed: do nothing */
        return 0;
    }
#if 0
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
    notice = 0;
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
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &allocators.raw);

    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &allocators.mem);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocators.obj);
    memory_trace_stop();
}


/*[clinic input]
_tracemalloc.start

    nframe: int = 1
    /

Start tracing Python memory allocations.

Also set the maximum number of frames stored in the traceback of a
trace to nframe.
[clinic start generated code]*/

static PyObject *
_tracemalloc_start_impl(PyObject *module, int nframe)
/*[clinic end generated code: output=caae05c23c159d3c input=40d849b5b29d1933]*/
{
    if (tracemalloc_start(nframe) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_tracemalloc.stop

Stop tracing Python memory allocations.

Also clear traces of memory blocks allocated by Python.
[clinic start generated code]*/

static PyObject *
_tracemalloc_stop_impl(PyObject *module)
/*[clinic end generated code: output=c3c42ae03e3955cd input=7478f075e51dae18]*/
{
    tracemalloc_stop();
    Py_RETURN_NONE;
}

static PyMethodDef module_methods[] = {
    _TRACEMALLOC_START_METHODDEF
    _TRACEMALLOC_STOP_METHODDEF
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

    if (tracemalloc_init() < 0)
        return NULL;

    return m;
}


int
_PyTraceMalloc_Init(int nframe)
{
    assert(PyGILState_Check());
    if (nframe == 0) {
        return 0;
    }
    return tracemalloc_start(nframe);
}


void
_PyTraceMalloc_Fini(void)
{
    assert(PyGILState_Check());
    tracemalloc_deinit();
}

int
PyTraceMalloc_Track(unsigned int domain, uintptr_t ptr,
                    size_t size)
{
    return 0;
}

int
PyTraceMalloc_Untrack(unsigned int domain, uintptr_t ptr)
{
    if (!tracemalloc_config.tracing) {
        /* tracemalloc is not tracing: do nothing */
        return -2;
    }

    return 0;
}


PyObject*
_PyTraceMalloc_GetTraceback(unsigned int domain, uintptr_t ptr)
{
   Py_INCREF(Py_None);
   return Py_None;
}

void
_PyMem_DumpTraceback(int fd, const void *ptr)
{
}

