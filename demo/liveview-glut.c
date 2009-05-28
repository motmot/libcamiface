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

#ifdef USE_GLEW
#  include <GL/glew.h>
#endif

#if defined(__APPLE__)
#  include <OpenGL/gl.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glut.h>
#endif
#include <math.h>

#include "cam_iface.h"

/* global variables */
CamContext *cc;
int width, height;
unsigned char *raw_pixels;
double buf_wf, buf_hf;
GLuint pbo, textureId;
int use_pbo;
int tex_width, tex_height;
size_t PBO_stride;

#define _check_error() {						\
    int _check_error_err;						\
    _check_error_err = cam_iface_have_error();				\
    if (_check_error_err != 0) {					\
      									\
      fprintf(stderr,"%s:%d %s\n", __FILE__,__LINE__,cam_iface_get_error_string()); \
      exit(1);								\
    }									\
  }									\

void yuv422_to_mono8(src_pixels, dest_pixels, width, height, src_stride, dest_stride) {
  int i,j;
  unsigned char* src_chunk, *dest_chunk;
  for (i=0; i<height; i++) {
    src_chunk = src_pixels + i*src_stride;
    dest_chunk = dest_pixels + i*dest_stride;
    for (j=0; j<(width/2); j++) {
      dest_chunk[0] = src_chunk[1];
      dest_chunk[1] = src_chunk[3];
      dest_chunk+=2;
      src_chunk+=4;
    }
  }
}

char* convert_pixels(char* src,
                   CameraPixelCoding src_coding,
                   size_t dest_stride,
                   char* dest, int force_copy) {
  static int gave_error=0;
  char* actual_dest;
  int i;
  int copy_required;

  copy_required = force_copy || (dest_stride!=width);

  if (src_coding==CAM_IFACE_MONO8) {
    if (copy_required) {
      // update data directly on the mapped buffer
      GLubyte* rowstart = dest;
      for (i=0; i<height; i++) {
        memcpy(rowstart, src + (width*i), width );
        rowstart += dest_stride;
      }
      return dest;
    } else {
      return src; /* no conversion necessary*/
    }
  } else if (src_coding==CAM_IFACE_YUV422) {
    yuv422_to_mono8(src, dest, width, height, width*2, dest_stride);
    return dest;
  } else {
    if (!gave_error) {
      fprintf(stderr,"ERROR: unsupported pixel coding %d\n",src_coding);
      gave_error=1;
    }
    if (copy_required) {
      // update data directly on the mapped buffer
      GLubyte* rowstart = dest;
      for (i=0; i<height; i++) {
        memcpy(rowstart, src + (width*i), width );
        rowstart += dest_stride;
      }
      return dest;
    } else {
      return src; /* no conversion necessary*/
    }
  }
}

void show_usage(char * cmd) {
  printf("usage: %s [num_frames]\n",cmd);
  printf("  where num_frames can be a number or 'forever'\n");
  exit(1);
}

double next_power_of_2(double f) {
  return pow(2.0,ceil(log(f)/log(2.0)));
}

void initialize_gl_texture() {
  char *buffer;

  if (use_pbo) {
    PBO_stride = ((width/32)*32);
    if (PBO_stride<width) PBO_stride+=32;
    printf("PBO stride: %d\n",PBO_stride);
    tex_width = PBO_stride;
    tex_height = height;
  } else {
    tex_width = (int)next_power_of_2(width);
    tex_height = (int)next_power_of_2(height);
  }

  buf_wf = ((double)(width))/((double)tex_width);
  buf_hf = ((double)(height))/((double)tex_height);

  printf("for %dx%d image, allocating %dx%d texture (fractions: %.2f, %.2f)\n",
         width,height,tex_width,tex_height,buf_wf,buf_hf);

  buffer = malloc( tex_height*tex_width );
  if (!buffer) {
    fprintf(stderr,"ERROR: failed to allocate buffer\n");
    exit(1);
  }

  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, /* target */
               0, /* mipmap level */
               GL_RGBA, /* internal format */
               tex_width, tex_height,
               0, /* border */
               GL_LUMINANCE, /* format */
               GL_UNSIGNED_BYTE, /* type */
               buffer);
  free(buffer);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void grab_frame(void); /* forward declaration */

void display_pixels() {
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);

    glNormal3d(0, 0, 1);

    glTexCoord2f(0,0);
    glVertex3f(-1,1,0);

    glTexCoord2f(0,buf_hf);
    glVertex3f(-1,-1,0);

    glTexCoord2f(buf_wf,buf_hf);
    glVertex3f(1,-1,0);

    glTexCoord2f(buf_wf,0);
    glVertex3f(1,1,0);

    glEnd();

    glutSwapBuffers();
}

int main(int argc, char** argv) {
  int device_number,ncams,num_buffers;

  double last_fps_print, now, t_diff;
  double fps;
  int buffer_size;
  int num_modes, num_props, num_trigger_modes;
  char mode_string[255];
  int i,mode_number;
  CameraPropertyInfo cam_props;
  long prop_value;
  int prop_auto;
  int left, top;
  cam_iface_constructor_func_t new_CamContext;
  Camwire_id cam_info_struct;

  glutInit(&argc, argv);

  cam_iface_startup_with_version_check();
  _check_error();

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

  glutInitWindowPosition(-1,-1);
  glutInitWindowSize(width, height);
  glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE );
  glutCreateWindow("libcamiface liveview");

#ifdef USE_GLEW
  glewInit();
  if (glewIsSupported("GL_VERSION_2_0 "
                      "GL_ARB_pixel_buffer_object")) {
    printf("PBO enabled\n");
    use_pbo=1;
  } else {
    printf("GLEW available, but no pixel buffer support -- not using PBO\n");
    use_pbo=0;
  }
#else
  printf("GLEW not available -- not using PBO\n");
  use_pbo=0;
#endif

  initialize_gl_texture();

  if (use_pbo) {
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB,
                 tex_width*tex_height, 0, GL_STREAM_DRAW);
  }

  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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
  raw_pixels = (unsigned char *)malloc( buffer_size );
  if (raw_pixels==NULL) {
    fprintf(stderr,"couldn't allocate memory in %s, line %d\n",__FILE__,__LINE__);
    exit(1);
  }
#endif

  glutDisplayFunc(display_pixels); /* set the display callback */
  glutIdleFunc(grab_frame); /* set the idle callback */

  CamContext_start_camera(cc);
  _check_error();

  printf("will now run forever. press Ctrl-C to interrupt\n");

  CamContext_get_num_trigger_modes( cc, &num_trigger_modes );
  _check_error();

  printf("trigger modes:\n");
  for (i =0; i<num_trigger_modes; i++) {
    CamContext_get_trigger_mode_string( cc, i, mode_string, 255 );
    printf("  %d: %s\n",i,mode_string);
  }
  printf("\n");

  glutMainLoop();
  printf("\n");
  delete_CamContext(cc);
  _check_error();

  cam_iface_shutdown();
  _check_error();

#ifdef USE_COPY
  free(raw_pixels);
#endif

  return 0;
}

/* Send the data to OpenGL. Use the fastest possible method. */

void upload_image_data_to_opengl(const char* raw_image_data,
                                 CameraPixelCoding coding) {
  int i;
  char * gl_image_data;
  static char* show_pixels=NULL;

  if (use_pbo) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, tex_width*tex_height, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      convert_pixels(raw_image_data, cc->coding, PBO_stride, ptr, 1);
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
    }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  } else {

    if (show_pixels==NULL) {
      /* allocate memory */
      show_pixels = (unsigned char *)malloc( width*height );
      if (show_pixels==NULL) {
        fprintf(stderr,"couldn't allocate memory in %s, line %d\n",__FILE__,__LINE__);
        exit(1);
      }
    }

    gl_image_data = convert_pixels(raw_image_data, cc->coding, width, show_pixels, 0);

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, /* target */
                    0, /* mipmap level */
                    0, /* x offset */
                    0, /* y offset */
                    width,
                    height,
                    GL_LUMINANCE, /* data format */
                    GL_UNSIGNED_BYTE, /* data type */
                    gl_image_data);

  }
}

/* grab_frame() is the idle-time callback function. It grabs an image,
   sends it to OpenGL, and tells GLUT to do the display function
   callback. */

void grab_frame(void) {
  int errnum;

#ifdef USE_COPY
    CamContext_grab_next_frame_blocking(cc,raw_pixels,-1.0f); // never timeout
    errnum = cam_iface_have_error();
    if (errnum == CAM_IFACE_FRAME_TIMEOUT) {
      cam_iface_clear_error();
      fprintf(stdout,"T");
      fflush(stdout);
      return; // wait again
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
    }

    upload_image_data_to_opengl(raw_pixels,cc->coding);

#else
    CamContext_point_next_frame_blocking(cc,&raw_pixels,-1.0f);
    _check_error();

    upload_image_data_to_opengl(raw_pixels,cc->coding);

    CamContext_unpoint_frame(cc);
    _check_error();
#endif

    glutPostRedisplay(); /* trigger display redraw */

}
