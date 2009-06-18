/*

Copyright (c) 2004-2009, California Institute of Technology. All
rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "cam_iface.h"

#include <ogg/ogg.h>
#include <theora/theora.h>

/* globals */
static FILE *ogg_fp = NULL;
static int theora_initialized = 0;
static ogg_stream_state ogg_os;
static theora_state theora_td;
static theora_info theora_ti;

double my_floattime() {
#ifdef _WIN32
#if _MSC_VER == 1310
  struct _timeb t;
  _ftime(&t);
  return (double)t.time + (double)t.millitm * (double)0.001;
#else
  struct _timeb t;
  if (_ftime_s(&t)==0) {
    return (double)t.time + (double)t.millitm * (double)0.001;
  }
  else {
    return 0.0;
  }
#endif
#else
  struct timeval t;
  if (gettimeofday(&t, (struct timezone *)NULL) == 0)
    return (double)t.tv_sec + t.tv_usec*0.000001;
  else
    return 0.0;
#endif
}

#define _check_error() {                                                \
    int _check_error_err;                                               \
    _check_error_err = cam_iface_have_error();                          \
    if (_check_error_err != 0) {                                        \
                                                                        \
      fprintf(stderr,"%s:%d %s\n", __FILE__,__LINE__,cam_iface_get_error_string()); \
      exit(1);                                                          \
    }                                                                   \
  }                                                                     \

void show_usage(char * cmd) {
  printf("usage: %s FILENAME [num_frames]\n",cmd);
  printf("  where FILENAME is the name of the .ogv theora file to write\n");
  printf("        num_frames can be a number or 'forever'\n");
  exit(1);
}

static int
theora_open(const char *pathname)
{
  ogg_packet op;
  ogg_page og;
  theora_comment tc;

  ogg_fp = fopen(pathname, "wb");
  if(!ogg_fp) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't open output file");
    return 1;
  }

  if(ogg_stream_init(&ogg_os, rand())) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't create ogg stream state");
    return 1;
  }

  if(theora_encode_init(&theora_td, &theora_ti)) {
    fprintf(stderr, "%s: error: %s\n",
      pathname, "couldn't initialize theora encoding");
    return 1;
  }

  theora_encode_header(&theora_td, &op);
  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  theora_comment_init(&tc);
  theora_encode_comment(&tc, &op);
  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  theora_encode_tables(&theora_td, &op);
  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  if(ogg_stream_flush(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  return 0;
}

typedef struct
{
  unsigned char R;
  unsigned char G;
  unsigned char B;
}
RGB888_t;

typedef struct
{
  unsigned char Y;
  unsigned char U;
  unsigned char V;
}
YUV444_t;

#define min(x,y)				\
  (x)<(y)?x:y

YUV444_t RGB888toYUV444(unsigned char r, unsigned char g, unsigned char b) {
  // from http://en.wikipedia.org/wiki/YUV
  YUV444_t result;
  result.Y = min(abs(r * 2104 + g * 4130 + b * 802 + 4096 + 131072) >> 13, 235);
  result.U = min(abs(r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576) >> 13, 240);
  result.V = min(abs(r * 3598 + g * -3013 + b * -585 + 4096 + 1048576) >> 13, 240) ;
  return result;
}

char * malloc_and_convert(char *input, CameraPixelCoding coding,
                          size_t input_stride, size_t w, size_t h) {
  char*output;
  size_t output_stride;
  int i,j;

  output_stride = w*3/2;
  output = (char*)malloc(output_stride*h);
  if (output==NULL) {
    return output;
  }

  switch(coding){
  case CAM_IFACE_RGB8:
    fprintf(stderr,"not doing real RGB->YUV422 conversion\n");
    for (i=0; i<h; i++) {
      memcpy(output + (i*output_stride),
             input + (i*input_stride),
             output_stride);
    }
    break;
  default:
    fprintf(stderr,"do not know how to convert image for this format\n");
    free(output);
    return NULL;
    break;
  }

  return output;
}

static int
theora_write_frame(unsigned long w, unsigned long h, unsigned char *yuv)
{
  yuv_buffer yuv_buf;
  ogg_packet op;
  ogg_page og;

  unsigned long yuv_w;
  unsigned long yuv_h;

  unsigned char *yuv_y;
  unsigned char *yuv_u;
  unsigned char *yuv_v;

  unsigned int x;
  unsigned int y;
  
  /* Must hold: yuv_w >= w */
  yuv_w = (w + 15) & ~15;

  /* Must hold: yuv_h >= h */
  yuv_h = (h + 15) & ~15;

  yuv_y = malloc(yuv_w * yuv_h);
  yuv_u = malloc(yuv_w * yuv_h / 4);
  yuv_v = malloc(yuv_w * yuv_h / 4);

  yuv_buf.y_width = yuv_w;
  yuv_buf.y_height = yuv_h;
  yuv_buf.y_stride = yuv_w;
  yuv_buf.uv_width = yuv_w >> 1;
  yuv_buf.uv_height = yuv_h >> 1;
  yuv_buf.uv_stride = yuv_w >> 1;
  yuv_buf.y = yuv_y;
  yuv_buf.u = yuv_u;
  yuv_buf.v = yuv_v;

  for(y = 0; y < yuv_h; y++) {
    for(x = 0; x < yuv_w; x++) {
      yuv_y[x + y * yuv_w] = 0;
    }
  }

  for(y = 0; y < yuv_h; y += 2) {
    for(x = 0; x < yuv_w; x += 2) {
      yuv_u[(x >> 1) + (y >> 1) * (yuv_w >> 1)] = 0;
      yuv_v[(x >> 1) + (y >> 1) * (yuv_w >> 1)] = 0;
    }
  }

  for(y = 0; y < h; y++) {
    for(x = 0; x < w; x++) {
      yuv_y[x + y * yuv_w] = yuv[3 * (x + y * w) + 0];
    }
  }

  for(y = 0; y < h; y += 2) {
    for(x = 0; x < w; x += 2) {
      yuv_u[(x >> 1) + (y >> 1) * (yuv_w >> 1)] =
        yuv[3 * (x + y * w) + 1];
      yuv_v[(x >> 1) + (y >> 1) * (yuv_w >> 1)] =
        yuv[3 * (x + y * w) + 2];
    }
  }

  if(theora_encode_YUVin(&theora_td, &yuv_buf)) {
    fprintf(stderr, "error: could not encode frame\n");
    return 1;
  }

  if(!theora_encode_packetout(&theora_td, 0, &op)) {
    fprintf(stderr, "error: could not read packets\n");
    return 1;
  }

  ogg_stream_packetin(&ogg_os, &op);
  if(ogg_stream_pageout(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  free(yuv_y);
  free(yuv_u);
  free(yuv_v);

  return 0;
}


static void
theora_close(void)
{
  ogg_packet op;
  ogg_page og;

  if (theora_initialized) {
    theora_encode_packetout(&theora_td, 1, &op);
    if(ogg_stream_pageout(&ogg_os, &og)) {
      fwrite(og.header, og.header_len, 1, ogg_fp);
      fwrite(og.body, og.body_len, 1, ogg_fp);
    }
  
    theora_info_clear(&theora_ti);
    theora_clear(&theora_td);
  
    fflush(ogg_fp);
    fclose(ogg_fp);
  }
  
  ogg_stream_clear(&ogg_os);
}

int main(int argc, char** argv) {
  CamContext *cc;
  unsigned char *pixels;

  int device_number,ncams,num_buffers;

  double last_fps_print, now, t_diff;
  double fps;
  int n_frames;
  int buffer_size;
  int num_modes, num_props, num_trigger_modes;
  char mode_string[255];
  int i,mode_number;
  CameraPropertyInfo cam_props;
  long prop_value;
  int prop_auto;
  int errnum;
  int left, top;
  int width, height;
  int do_num_frames;
  cam_iface_constructor_func_t new_CamContext;
  Camwire_id cam_info_struct;
  char *filename;
  unsigned char *yuv;

  cam_iface_startup_with_version_check();
  _check_error();

  if (argc<1) {
    show_usage(argv[0]);
  }

  filename = argv[1];

  if (argc>2) {
    if (strcmp(argv[2],"forever")==0) {
      do_num_frames = -1;
    } else if (sscanf(argv[2],"%d",&do_num_frames)==0) {
      show_usage(argv[0]);
    }
  } else {
    do_num_frames = -1;
  }

  for (i=0;i<argc;i++) {
    printf("%d: %s\n",i,argv[i]);
  }
  printf("using driver %s\n",cam_iface_get_driver_name());

  ncams = cam_iface_get_num_cameras();
  _check_error();

  if (ncams<1) {

    printf("no cameras found, will now exit\n");

    cam_iface_shutdown();
    _check_error();

    exit(1);
  }
  _check_error();

  printf("%d camera(s) found.\n",ncams);
  for (i=0; i<ncams; i++) {
    cam_iface_get_camera_info(i, &cam_info_struct);
    printf("  camera %d:\n",i);
    printf("    vendor: %s\n",cam_info_struct.vendor);
    printf("    model: %s\n",cam_info_struct.model);
    printf("    chip: %s\n",cam_info_struct.chip);
  }

  device_number = ncams-1;

  printf("choosing camera %d\n",device_number);

  cam_iface_get_num_modes(device_number, &num_modes);
  _check_error();

  printf("%d mode(s) available:\n",num_modes);

  mode_number = 0;

  for (i=0; i<num_modes; i++) {
    cam_iface_get_mode_string(device_number,i,mode_string,255);
    if (strstr(mode_string,"FORMAT7_0")!=NULL) {
      if (strstr(mode_string,"MONO8")!=NULL) {
        // pick this mode
        mode_number = i;
      }
    }
    printf("  %d: %s\n",i,mode_string);
  }

  printf("Choosing mode %d\n",mode_number);

  num_buffers = 5;

  new_CamContext = cam_iface_get_constructor_func(device_number);
  cc = new_CamContext(device_number,num_buffers,mode_number);
  _check_error();

  CamContext_get_frame_roi(cc, &left, &top, &width, &height);
  _check_error();

    if(!theora_initialized) {
      theora_info_init(&theora_ti);

      theora_ti.width = ((width + 15) >>4)<<4;
      theora_ti.height = ((height + 15)>>4)<<4;
      theora_ti.frame_width = width;
      theora_ti.frame_height = height;
      theora_ti.offset_x = 0;
      theora_ti.offset_y = 0;
      theora_ti.fps_numerator = 30;
      theora_ti.fps_denominator = 1;
      theora_ti.aspect_numerator = width;
      theora_ti.aspect_denominator = height;
      theora_ti.colorspace = OC_CS_UNSPECIFIED;
      theora_ti.pixelformat = OC_PF_420;
      theora_ti.target_bitrate = 0;
      theora_ti.quality = 40; /* between 0 and 63 */

      theora_ti.dropframes_p = 0;
      theora_ti.quick_p = 1;
      theora_ti.keyframe_auto_p = 1;
      theora_ti.keyframe_frequency = 64;
      theora_ti.keyframe_frequency_force = 64;
      theora_ti.keyframe_data_target_bitrate = 0;
      theora_ti.keyframe_mindistance = 8;
      theora_ti.noise_sensitivity = 1;


      if(theora_open(filename)) {
        cam_iface_shutdown();
        _check_error();

        exit(1);
      }
    }


  CamContext_get_num_framebuffers(cc,&num_buffers);
  printf("allocated %d buffers\n",num_buffers);

  CamContext_get_num_camera_properties(cc,&num_props);
  _check_error();

  for (i=0; i<num_props; i++) {
    CamContext_get_camera_property_info(cc,i,&cam_props);
    _check_error();

    if (strcmp(cam_props.name,"white balance")==0) {
      fprintf(stderr,"WARNING: ignoring white balance property\n");
      continue;
    }

    if (cam_props.is_present) {
      CamContext_get_camera_property(cc,i,&prop_value,&prop_auto);
      _check_error();
      printf("  %s: %ld\n",cam_props.name,prop_value);
    } else {
      printf("  %s: not present\n",cam_props.name);
    }
  }

  CamContext_get_buffer_size(cc,&buffer_size);
  _check_error();

  if (buffer_size == 0) {
    fprintf(stderr,"buffer size was 0 in %s, line %d\n",__FILE__,__LINE__);
    exit(1);
  }

#define USE_COPY
#ifdef USE_COPY
  pixels = (unsigned char *)malloc( buffer_size );
  if (pixels==NULL) {
    fprintf(stderr,"couldn't allocate memory in %s, line %d\n",__FILE__,__LINE__);
    exit(1);
  }
#endif

  CamContext_start_camera(cc);
  _check_error();

  last_fps_print = my_floattime();
  n_frames = 0;

  if (do_num_frames < 0) {
    printf("will now run forever. press Ctrl-C to interrupt\n");
  } else {
    printf("will now grab %d frames.\n",do_num_frames);
  }

  CamContext_get_num_trigger_modes( cc, &num_trigger_modes );
  _check_error();

  printf("trigger modes:\n");
  for (i =0; i<num_trigger_modes; i++) {
    CamContext_get_trigger_mode_string( cc, i, mode_string, 255 );
    printf("  %d: %s\n",i,mode_string);
  }
  printf("\n");

  while (1) {
    if (do_num_frames>=0) {
      do_num_frames--;
      if (do_num_frames<0) break;
    }
#ifdef USE_COPY
    //CamContext_grab_next_frame_blocking(cc,pixels,0.2); // timeout after 200 msec
    CamContext_grab_next_frame_blocking(cc,pixels,-1.0f); // never timeout
    errnum = cam_iface_have_error();
    if (errnum == CAM_IFACE_FRAME_TIMEOUT) {
      cam_iface_clear_error();
      fprintf(stdout,"T");
      fflush(stdout);
      continue; // wait again
    }
    if (errnum == CAM_IFACE_FRAME_DATA_MISSING_ERROR) {
      cam_iface_clear_error();
      fprintf(stdout,"M");
      fflush(stdout);
    } else if (errnum == CAM_IFACE_FRAME_INTERRUPTED_SYSCALL) {
      cam_iface_clear_error();
      fprintf(stdout,"I");
      fflush(stdout);
    } else {
      _check_error();
      fprintf(stdout,".");
      fflush(stdout);
    }
    now = my_floattime();
    n_frames += 1;
#else
    CamContext_point_next_frame_blocking(cc,&pixels,-1.0f);
    now = my_floattime();
    n_frames += 1;
    _check_error();
    fprintf(stdout,".");
    fflush(stdout);
    CamContext_unpoint_frame(cc);
    _check_error();
#endif

    t_diff = now-last_fps_print;
    if (t_diff > 5.0) {
      fps = n_frames/t_diff;
      fprintf(stdout,"%.1f fps\n",fps);
      last_fps_print = now;
      n_frames = 0;
    }

    yuv = malloc_and_convert(pixels,cc->coding,width*3,width,height);
    if (yuv==NULL) {
      theora_close();
      cam_iface_shutdown();
      _check_error();
      exit(1);
    }
    if(theora_write_frame(width, height, yuv)) {
      theora_close();
      cam_iface_shutdown();
      _check_error();
      exit(1);
    }
    free(yuv);

  }
  theora_close();

  printf("\n");
  delete_CamContext(cc);
  _check_error();

  cam_iface_shutdown();
  _check_error();

#ifdef USE_COPY
  free(pixels);
#endif

  return 0;
}
