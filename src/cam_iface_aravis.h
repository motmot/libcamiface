/* internal structures for dc1394 implementation */

#ifdef MEGA_BACKEND
  #define BACKEND_METHOD(m) aravis_##m
#else
  #define BACKEND_METHOD(m) m
#endif

#include "cam_iface_static_functions.h"
