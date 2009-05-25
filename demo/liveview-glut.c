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

#if defined(__APPLE__)
#ifdef USE_GLEW
#  include <GLEW/glew.h>
#endif
#  include <OpenGL/gl.h>
#  include <GLUT/glut.h>
#else
#ifdef USE_GLEW
#  include <GL/glew.h>
#endif
#  include <GL/gl.h>
#  include <GL/glut.h>
#endif
#include <math.h>

#include "cam_iface.h"

/* global variables */
CamContext *cc;
int width, height;
unsigned char *raw_pixels, *converted_pixels, *show_pixels;
double buf_wf, buf_hf;
GLuint pbo, textureId;
int use_pbo;
int tex_width, tex_height;
int rowsize;

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

unsigned char* convert_pixels(CameraPixelCoding coding) {
  static int gave_error=0;
  if (coding==CAM_IFACE_MONO8) {
    return raw_pixels; /* no conversion necessary*/
  } else if (coding==CAM_IFACE_YUV422) {
    yuv422_to_mono8(raw_pixels, converted_pixels, width, height, width*2, width);
    return converted_pixels; /* don't convert -- but what we show will be wrong */
  } else {
    if (!gave_error) {
      fprintf(stderr,"ERROR: unsupported pixel coding %d\n",coding);
      gave_error=1;
    }
    return raw_pixels; /* don't convert -- but what we show will be wrong */
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
    tex_width = width;
    tex_height = height;
  } else {
    tex_width = (int)next_power_of_2(width);
    tex_height = (int)next_power_of_2(height);
  }
  rowsize= ((width/32)*32);
  if (rowsize<width) rowsize+=32;

  buf_wf = ((double)(width))/((double)tex_width);
  buf_hf = ((double)(height))/((double)tex_height);

  printf("for %dx%d image, allocating %dx%d texture (fractions: %.2f, %.2f)\n",
         width,height,tex_width,tex_height,buf_wf,buf_hf);

  buffer = malloc( tex_height*tex_width );
  if (!buffer) {
    fprintf(stderr,"ERROR: failed to allocate buffer\n");
    exit(1);
  }

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

  converted_pixels = (unsigned char *)malloc( width*height );
  if (converted_pixels==NULL) {
    fprintf(stderr,"couldn't allocate memory in %s, line %d\n",__FILE__,__LINE__);
    exit(1);
  }
  show_pixels = raw_pixels;

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
  free(converted_pixels);

  return 0;
}

void upload_image_data_to_opengl(const char* image_data,
                                 CameraPixelCoding coding) {
  show_pixels = convert_pixels(cc->coding);
  int i;
  if (use_pbo) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width*height, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr)
      {
        // update data directly on the mapped buffer
        GLubyte* rowstart = ptr;
        for (i=0; i<height; i++) {
          memcpy(rowstart, show_pixels + (width*i), width );
          rowstart += rowsize;
        }
        glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
      }
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  } else {

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, /* target */
                    0, /* mipmap level */
                    0, /* x offset */
                    0, /* y offset */
                    width,
                    height,
                    GL_LUMINANCE, /* data format */
                    GL_UNSIGNED_BYTE, /* data type */
                    show_pixels);

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
