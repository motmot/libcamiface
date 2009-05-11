#ifdef _MSC_VER
  //#define cam_iface_thread_local __declspec(thread)
#define cam_iface_thread_local
#else
#ifdef __APPLE__
// See the following for a hint on how to make thread thread-local without __thread.
// http://lists.apple.com/archives/Xcode-users/2006/Jun/msg00551.html
#define cam_iface_thread_local
#warning "Thread local storage not implemented"
#else
#define cam_iface_thread_local __thread
#endif
#endif


#ifdef _WIN32
#if _MSC_VER == 1310
#define cam_iface_snprintf(dst, len, fmt, ...) _snprintf((char*)dst, (size_t)len, (const char*)fmt, __VA_ARGS__)
#else
#define cam_iface_snprintf(dst, len, fmt, ...) _snprintf_s((char*)dst, (size_t)len, (size_t)len, (const char*)fmt, __VA_ARGS__)
#endif
#else
#define cam_iface_snprintf(...) snprintf(__VA_ARGS__)
#endif
