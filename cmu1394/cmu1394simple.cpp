#include <windows.h>
#include <WinError.h>
#include "1394Camera.h"
#include <stdio.h>

#ifdef TESTFAIL
#include <sys/timeb.h>
#include <time.h>

double floattime() {
  struct _timeb t;
  _ftime(&t);
  return (double)t.time + (double)t.millitm * (double)0.001;
}
#endif



#define CHK(m)								\
  if ( (m) != CAM_SUCCESS) {						\
    fprintf(stderr,"error in file %s, line %d\n",__FILE__,__LINE__);	\
    fflush(stderr);							\
    exit(1);								\
  }

int main() {
  int num_cameras;
  C1394Camera* cam;
  cam = new C1394Camera;

  printf("time is now %f\n",floattime());
  CHK(cam->CheckLink());

  num_cameras = cam->GetNumberCameras();
  printf("%d cameras\n",num_cameras);

  if (!num_cameras) {
    exit(0);
  }

  CHK(cam->CheckLink());
  CHK(cam->SelectCamera(0));
  CHK(cam->InitCamera());
  CHK(cam->SetVideoFormat(7));

  printf("time is now %f\n",floattime());
}
