/* internal structures for basler_pylon implementation */

#ifdef MEGA_BACKEND
  #define BACKEND_METHOD(m) basler_pylon_##m
#else
  #define BACKEND_METHOD(m) m
#endif

#include "cam_iface_static_functions.h"
