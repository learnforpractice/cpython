# cython: c_string_type=str, c_string_encoding=ascii
from libcpp.string cimport string
import time

def test():
    print('++++++++++++++++++hello,master')
time_start = 0
cdef extern void timeit_start():
    global time_start
    time_start = time.time()

cdef extern double timeit_end():
    return time.time() - time_start

