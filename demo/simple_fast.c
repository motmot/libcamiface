#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include <errno.h>
#include <sched.h>
#endif
#include <time.h>
#include "cam_iface.h"

double floattime() {
#ifdef _WIN32
  struct _timeb t;
  _ftime(&t);
  return (double)t.time + (double)t.millitm * (double)0.001;
#else
  struct timeval t;
  if (gettimeofday(&t, (struct timezone *)NULL) == 0)
    return (double)t.tv_sec + t.tv_usec*0.000001;
  else
    return 0.0;
#endif
}

/*
void _check_error() {
  int err;

  err = cam_iface_have_error();
  if (err != 0) {

    fprintf(stderr,cam_iface_get_error_string());
    fprintf(stderr,"\n");
    exit(1);
  }
}
*/
#define cam_iface_err_check(m) \
  if (cam_iface_have_error()) {					\
    fprintf(stderr,"%s (%d): %s\n",				\
	    __FILE__,__LINE__,cam_iface_get_error_string());	\
    fflush(stderr);						\
    exit(1);							\
}

int set_max_priority() {
#ifdef _WIN32
  return 0;
#else
  struct sched_param params;
  int priority;
  int retval;
  int savederr;

  priority = sched_get_priority_max( SCHED_FIFO );
  params.sched_priority = priority;
  
  errno = 0;
  retval = sched_setscheduler(0,SCHED_FIFO,&params);
  if (errno) {
    savederr = errno; /* prevent modification */
    fprintf(stderr, "ERROR: %s\n", strerror(savederr));
    fflush(stderr);
    exit(1);
  }
  return retval;
#endif
}

int main() {
  CamContext *cc;
  unsigned char *pixels;

  double last_fps_print, now, t_diff;
  double fps;
  int n_frames;
  int buffer_size;
  
  cam_iface_startup();
  cam_iface_err_check("");

  cc = new_CamContext(0,120,0);
  cam_iface_err_check("");
  
  CamContext_set_camera_property(cc, SHUTTER, 10, 0, 0);
  cam_iface_err_check("");

  CamContext_set_frame_size( cc, 64, 64);
  cam_iface_err_check("");

  CamContext_set_framerate( cc, 500.0);
  cam_iface_err_check("");

  //set_max_priority();

  CamContext_get_buffer_size(cc,&buffer_size);

  pixels = (unsigned char *)malloc( buffer_size );
  if (pixels==NULL) {
    fprintf(stderr,"couldn't allocate memory\n");
    exit(1);
  }

  CamContext_start_camera(cc);
  cam_iface_err_check("");

  last_fps_print = floattime();
  n_frames = 0;
  for (;;) {
#define USE_COPY
#ifdef USE_COPY
    CamContext_grab_next_frame_blocking(cc,pixels);
    now = floattime();
    n_frames += 1;
    cam_iface_err_check("");
    fprintf(stdout,".");
    fflush(stdout);
#else
    CamContext_point_next_frame_blocking(cc,&pixels);
    now = floattime();
    n_frames += 1;
    cam_iface_err_check("");
    /* fprintf(stdout,".");
      fflush(stdout);*/
    CamContext_unpoint_frame(cc);
    cam_iface_err_check("");
#endif
    t_diff = now-last_fps_print;
    if (t_diff > 5.0) {
      fps = n_frames/t_diff;
      fprintf(stdout,"%.1f fps\n",fps);
      last_fps_print = now;
      n_frames = 0;
      }
  }

  delete_CamContext(cc);
  cam_iface_err_check("");
  cam_iface_shutdown();
}
