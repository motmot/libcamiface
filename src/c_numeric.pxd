#emacs, this is -*-Python-*- mode

cdef extern from "Numeric/arrayobject.h":
    ctypedef enum PyArray_TYPES:
        PyArray_UBYTE
    void import_array()
##    ctypedef struct PyArrayObject:
##        char * data
##        int nd
##        int *dimensions
##        int *strides
    ctypedef class Numeric.ArrayType [object PyArrayObject]:
        cdef char *data
        cdef int nd
        cdef int *dimensions, *strides

##    PyArrayObject* PyArray_ContiguousFromObject( object o, int type_num,
##                                         int min_dimensions,
##                                         int max_dimensions )

##    PyArrayObject* PyArray_FromDims(int n_dimensions,
##                                    int* dimensions,
##                                    int type_num)
    object PyArray_ContiguousFromObject( object o, int type_num,
                                         int min_dimensions,
                                         int max_dimensions )

    object PyArray_FromDims(int n_dimensions,
                            int* dimensions,
                            int type_num)
