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
