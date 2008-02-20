/* $Id: $ */
#include "Python.h"

#include "cam_iface.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

/* globals -- allocate space */
char* argv0="";
static int py_initialized=0;
static PyObject *mod_pynet=NULL;
static PyObject *py_new_CamContext = NULL;
static PyObject *py_close_CamContext = NULL;
static PyObject *py_get_num_cameras = NULL;
static PyObject *py_get_num_modes = NULL;
static PyObject *py_get_camera_info = NULL;
static PyObject *py_get_mode_string = NULL;

int cam_iface_error = 0;
#define CAM_IFACE_MAX_ERROR_LEN 255
char cam_iface_error_string[CAM_IFACE_MAX_ERROR_LEN]  = {0x00}; //...

struct CCpynet; // forward declaration

// keep functable in sync across backends
typedef struct {
  cam_iface_constructor_func_t construct;
  void (*destruct)(struct CamContext*);

  void (*CCpynet)(struct CCpynet*,int,int,int);
  void (*close)(struct CCpynet*);
  void (*start_camera)(struct CCpynet*);
  void (*stop_camera)(struct CCpynet*);
  void (*get_num_camera_properties)(struct CCpynet*,int*);
  void (*get_camera_property_info)(struct CCpynet*,
				   int,
				   CameraPropertyInfo*);
  void (*get_camera_property)(struct CCpynet*,int,long*,int*);
  void (*set_camera_property)(struct CCpynet*,int,long,int);
  void (*grab_next_frame_blocking)(struct CCpynet*,
				   unsigned char*,
				   float);
  void (*grab_next_frame_blocking_with_stride)(struct CCpynet*,
					       unsigned char*,
					       intptr_t,
					       float);
  void (*point_next_frame_blocking)(struct CCpynet*,unsigned char**,float);
  void (*unpoint_frame)(struct CCpynet*);
  void (*get_last_timestamp)(struct CCpynet*,double*);
  void (*get_last_framenumber)(struct CCpynet*,long*);
  void (*get_num_trigger_modes)(struct CCpynet*,int*);
  void (*get_trigger_mode_string)(struct CCpynet*,int,char*,int);
  void (*get_trigger_mode_number)(struct CCpynet*,int*);
  void (*set_trigger_mode_number)(struct CCpynet*,int);
  void (*get_frame_offset)(struct CCpynet*,int*,int*);
  void (*set_frame_offset)(struct CCpynet*,int,int);
  void (*get_frame_size)(struct CCpynet*,int*,int*);
  void (*set_frame_size)(struct CCpynet*,int,int);
  void (*get_max_frame_size)(struct CCpynet*,int*,int*);
  void (*get_buffer_size)(struct CCpynet*,int*);
  void (*get_framerate)(struct CCpynet*,float*);
  void (*set_framerate)(struct CCpynet*,float);
  void (*get_num_framebuffers)(struct CCpynet*,int*);
  void (*set_num_framebuffers)(struct CCpynet*,int);
} CCpynet_functable;

// this is a static variable which we fill in from Python
CCpynet_functable CCpynet_vmt = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

typedef struct CCpynet {
  CamContext inherited;
  PyObject *self;
} CCpynet;


// ====================================

#ifdef _WIN32
#if _MSC_VER == 1310
#define cam_iface_snprintf(dst, len, fmt, ...) _snprintf((char*)dst, (size_t)len, (const char*)fmt, __VA_ARGS__)
#else
#define cam_iface_snprintf(dst, len, fmt, ...) _snprintf_s((char*)dst, (size_t)len, (size_t)len, (const char*)fmt, __VA_ARGS__)
#endif
#else
#define cam_iface_snprintf(...) snprintf(__VA_ARGS__)
#endif

#define CAM_IFACE_ERROR_FORMAT(m) \
cam_iface_snprintf(cam_iface_error_string,CAM_IFACE_MAX_ERROR_LEN,"%s (%d): %s\n",__FILE__,__LINE__,(m));

#define CHECK_CC(m)							\
  if (!(m)) {								\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("no CamContext specified (NULL argument)");	\
    return;								\
  }

// Check a PyObject* (or check 0 to force a Python error)
#define CHECK_PY(m)							\
  if (!(m)) {								\
    PyErr_Print();							\
    cam_iface_error = -1;						\
    CAM_IFACE_ERROR_FORMAT("Python exception");				\
    return;								\
  }

#define CAM_IFACE_THROW_ERROR(m)			\
  {							\
    cam_iface_error = -1;				\
    CAM_IFACE_ERROR_FORMAT((m));			\
    return;						\
  }

#define NOT_IMPLEMENTED							\
  cam_iface_error = -1;							\
  CAM_IFACE_ERROR_FORMAT("Function not implemented.");			\
  return;

double ci_pynet_floattime() {
  struct timeval t;
  if (gettimeofday(&t, (struct timezone *)NULL) == 0)
    return (double)t.tv_sec + t.tv_usec*0.000001;
  else
    return 0.0;
}

// =====================================

const char *cam_iface_get_driver_name() {
  return "pynet";
}

void cam_iface_clear_error() {
  cam_iface_error = 0;
}

int cam_iface_have_error() {
  if (py_initialized) {
    if (PyErr_Occurred()) { CHECK_PY(NULL); }
  }
  return cam_iface_error;
}

const char * cam_iface_get_error_string() {
  return cam_iface_error_string;
}

const char* cam_iface_get_api_version() {
  return CAM_IFACE_API_VERSION;
}

extern void cam_iface_startup() {
  PyObject* _set_vmt_ptr_func=NULL;
  PyObject* tmp=NULL;
  PyObject* arglist=NULL;
  PyObject* pyresult=NULL;

  // Py_SetProgramName("cam_iface_pynet");

  /* Initialize the Python interpreter.  Required. */
  Py_Initialize();
  py_initialized=1;

  PyRun_SimpleString("import sys\n"
		     "sys.path.insert(0,'/usr/share/libcamiface')\n"
		     "sys.path.insert(0,'.')\n"
		     );

  // PySys_SetArgv(1,&argv0); // This crashes if running in interpreter.

  tmp = PyString_FromString("pynetx"); // This is the name of the Python module we want
  CHECK_PY(tmp);
  mod_pynet = PyImport_Import(tmp); // new ref
  Py_DECREF(tmp);
  CHECK_PY(mod_pynet);

  _set_vmt_ptr_func = PyObject_GetAttrString(mod_pynet, "_set_vmt_ptr_and_size");
  CHECK_PY(_set_vmt_ptr_func);

  tmp=PyCObject_FromVoidPtr((void*)&CCpynet_vmt,NULL); // new ref
  CHECK_PY(tmp);

  arglist = Py_BuildValue("(Ni)", tmp, sizeof(CCpynet_functable));
  CHECK_PY(arglist);

  pyresult = PyObject_CallObject(_set_vmt_ptr_func, arglist);
  CHECK_PY(pyresult);
  Py_DECREF(pyresult);
  Py_DECREF(arglist);
  Py_DECREF(tmp);

  py_get_num_cameras = PyObject_GetAttrString(mod_pynet, "get_num_cameras");
  CHECK_PY(py_get_num_cameras);

  py_get_num_modes = PyObject_GetAttrString(mod_pynet, "get_num_modes");
  CHECK_PY(py_get_num_modes);

  py_get_camera_info = PyObject_GetAttrString(mod_pynet, "get_camera_info");
  CHECK_PY(py_get_camera_info);

  py_get_mode_string = PyObject_GetAttrString(mod_pynet, "get_mode_string");
  CHECK_PY(py_get_mode_string);

  py_new_CamContext = PyObject_GetAttrString(mod_pynet, "new_CamContextPy");
  CHECK_PY(py_new_CamContext);

  py_close_CamContext = PyObject_GetAttrString(mod_pynet, "close_CamContextPy");
  CHECK_PY(py_close_CamContext);

  tmp = PyObject_GetAttrString(mod_pynet, "initialize");
  CHECK_PY(tmp);

  arglist = Py_BuildValue("()");
  CHECK_PY(arglist);

  pyresult = PyObject_CallObject(tmp, arglist);
  CHECK_PY(pyresult);
  Py_DECREF(pyresult);
  Py_DECREF(arglist);
  Py_DECREF(tmp);

}

extern void cam_iface_shutdown() {
  Py_XDECREF(mod_pynet);
  Py_XDECREF(py_get_num_cameras);  // Dispose of callback
  Py_XDECREF(py_get_num_modes);  // Dispose of callback
  Py_XDECREF(py_get_camera_info);  // Dispose of callback
  Py_XDECREF(py_get_mode_string);  // Dispose of callback
  Py_XDECREF(py_new_CamContext);  // Dispose of callback
  Py_XDECREF(py_close_CamContext);  // Dispose of callback

  Py_Finalize();
  py_initialized=0;
}


int cam_iface_get_num_cameras() {
  PyObject *pValue, *pModulePy;
  int result;

  CHECK_PY(py_get_num_cameras);
  pValue = PyObject_CallObject(py_get_num_cameras, NULL);
  CHECK_PY(pValue);

  result = (int)PyInt_AsLong(pValue);
  if (PyErr_Occurred()) { CHECK_PY(NULL); }
  Py_DECREF(pValue);
  return result;
}

void cam_iface_get_camera_info(int device_number, Camwire_id *out_camid) {
  PyObject *pValue, *pModulePy, *arglist;
  char* newstring;
  Py_ssize_t length;
  int result;

  CHECK_PY(py_get_camera_info);

  arglist = Py_BuildValue("(i)",device_number);
  CHECK_PY(arglist);

  pValue = PyObject_CallObject(py_get_camera_info, arglist);
  CHECK_PY(pValue);

  /*
  if (PyString_AsStringAndSize(pValue,&newstring,&length)==-1) {
    return;
  }
  */
  snprintf( out_camid->vendor, CAMWIRE_ID_MAX_CHARS, "pynet backend: vendor not implemented");
  snprintf( out_camid->model, CAMWIRE_ID_MAX_CHARS, "pynet backend: model not implemented");
  snprintf( out_camid->chip, CAMWIRE_ID_MAX_CHARS, "pynet backend: chip not implemented");
  //cam_iface_snprintf(mode_string,mode_string_maxlen,"(blank mode string)");
}


void cam_iface_get_num_modes(int device_number, int *num_modes) {
  PyObject *pValue, *pModulePy, *arglist;
  int result;

  CHECK_PY(py_get_num_modes);

  arglist = Py_BuildValue("(i)",device_number);
  CHECK_PY(arglist);

  pValue = PyObject_CallObject(py_get_num_modes, arglist);
  CHECK_PY(pValue);

  result = (int)PyInt_AsLong(pValue);
  if (PyErr_Occurred()) { CHECK_PY(NULL); }
  Py_DECREF(pValue);
  Py_DECREF(arglist);
  *num_modes = result;
}

void cam_iface_get_mode_string(int device_number,
			       int mode_number,
			       char* mode_string,
			       int mode_string_maxlen) {
  PyObject *pValue, *pModulePy, *arglist;
  char* newstring;
  Py_ssize_t length;
  int result;

  CHECK_PY(py_get_num_modes);
  printf("pynet get_mode_string called\n");
  arglist = Py_BuildValue("(ii)",device_number,mode_number);
  CHECK_PY(arglist);

  pValue = PyObject_CallObject(py_get_mode_string, arglist);
  CHECK_PY(pValue);

  if (PyString_AsStringAndSize(pValue,&newstring,&length)==-1) {
    return;
  }

  snprintf( mode_string, mode_string_maxlen, "%s", newstring );
  //cam_iface_snprintf(mode_string,mode_string_maxlen,"(blank mode string)");
}

// ====================
// Implement some of the constructor/destructor stuff that is hard to do in Python

// forward declaration
void delete_CCpynet( CCpynet* this);
void CCpynet_close( CCpynet* this);
void CCpynet_CCpynet( CCpynet* this, int device_number, int NumImageBuffers,
		      int mode_number);

CCpynet* CCpynet_construct( int device_number, int NumImageBuffers,
			    int mode_number);

cam_iface_constructor_func_t cam_iface_get_constructor_func(int device_number) {
  return (cam_iface_constructor_func_t)CCpynet_construct;
}

CCpynet* CCpynet_construct( int device_number, int NumImageBuffers,
			      int mode_number) {
  CCpynet* this=NULL;

  this = malloc(sizeof(CCpynet));
  bzero(this,sizeof(CCpynet));

  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("error allocating memory");
  } else {
    CCpynet_CCpynet(this, device_number, NumImageBuffers, mode_number);
  }

  return this;
}

void CCpynet_CCpynet( CCpynet* this, int device_number, int NumImageBuffers,
		      int mode_number) {
  PyObject* arg=NULL;
  PyObject* arglist=NULL;
  PyObject* result=NULL;
  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("passed NULL CCpynet instance");
  } else {
    CHECK_PY(py_new_CamContext);

    // Time to call the callback
    arg=PyCObject_FromVoidPtr((void*)this,NULL); // new ref
    CHECK_PY(arg);

    arglist = Py_BuildValue("(Niii)", arg,device_number,NumImageBuffers,mode_number);
    CHECK_PY(arglist);
    result = PyEval_CallObject(py_new_CamContext, arglist);
    Py_DECREF(arg);
    Py_DECREF(arglist);
    CHECK_PY(result);
    Py_DECREF(result);
    this->inherited.vmt = (CamContext_functable*)&CCpynet_vmt;
    // XXX TODO fix this hack:
    CCpynet_vmt.destruct = delete_CCpynet;
    CCpynet_vmt.close = CCpynet_close;
  }
}

void CCpynet_close( CCpynet* this) {
  PyObject* arg=NULL;
  PyObject* arglist=NULL;
  PyObject* result=NULL;
  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("passed NULL CCpynet instance");
  } else {
    CHECK_PY(py_close_CamContext);

    // Time to call the callback
    arg=PyCObject_FromVoidPtr((void*)this,NULL); // new ref
    CHECK_PY(arg);

    arglist = Py_BuildValue("(N)", arg);
    CHECK_PY(arglist);
    result = PyEval_CallObject(py_close_CamContext, arglist);
    Py_DECREF(arg);
    Py_DECREF(arglist);
    CHECK_PY(result);
    Py_DECREF(result);
  }
}

void delete_CCpynet( CCpynet* this) {
  if (this==NULL) {
    cam_iface_error = -1;
    CAM_IFACE_ERROR_FORMAT("passed NULL CCpynet instance");
  } else {
    CCpynet_close(this);
    free(this);
  }
}
