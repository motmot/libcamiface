#include "Python.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

static void* vmt_ptr=NULL;
static int vmt_size=0;

#define CHECK_PY(m)				\
  if (!(m)) {					\
    goto error;					\
  }

static PyObject* _set_vmt_ptr_and_size(PyObject *dummy, PyObject *args) {
  PyObject *result = NULL;
  PyObject *pycobjptr = NULL;
  if (PyArg_ParseTuple(args, "Oi:_set_vmt_ptr_and_size",&pycobjptr,&vmt_size)) {
    CHECK_PY(pycobjptr);
    if (PyCObject_Check(pycobjptr)) {
      vmt_ptr = PyCObject_AsVoidPtr(pycobjptr);
      Py_INCREF(Py_None);
      result = Py_None;
    }
  }
  return result;
 error:
  return NULL;
}

void c_set_vmt(void* src_ptr) {
  memcpy( vmt_ptr, src_ptr, vmt_size );
}

static PyMethodDef _pynetMethods[] = {
    {"_set_vmt_ptr_and_size", _set_vmt_ptr_and_size, METH_VARARGS,
     "(Call from embedded interpreter to set location of virtual methods table.)"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_pynet(void)
{
  (void) Py_InitModule("_pynet", _pynetMethods);
  return;
}
