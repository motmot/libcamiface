/***********************************************************************

    This file is in the public domain.

    Compile and link with something like:

    $ gcc -g -O2 -Wall -W -I/usr/local/share/linux/include -L/usr/local/share/linux/lib/camwire -o latency latency.c -lcamwire_1394 -ldc1394_control -lraw1394

    Title:  Simple libdc1394 latency meter
    Author: Johann Schoonees

    $Id: camlatency.c,v 7.0 2005/02/18 02:09:14 johanns Exp $

    Description:

    Measures the frame latency (time between start of frame capture and
    its availability) in libdc1394.  More precisely, it measures the
    minimum, maximum, and median time difference between the moment a
    blocking frame capture call returns, and the timestamp reported by
    Camwire which is presumably the moment frame capture was triggered.
    Uses the Camwire generic API and assumes that the camera is
    initialized to the wanted state (particularly shutter time and frame
    rate) through the Camwire initialization file.  The measured latency
    will also be affected by the numbers in the Camwire configuration
    file, which are used in the timestamp calculation.

***********************************************************************/


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <float.h>

#include "camwire/camwire.h"


typedef struct stats_t
{
    double min, max, median, fps;
} stats_t;
    

static void get_command_line(int argc, char *argv[], int *curr_cam,
			     long int *n_iter);
static void measure_latency(const Camwire_handle c_handle,
			    const long int num_iter, stats_t *s);
static int sort_compare(const void *a, const void *b);
static void wait_frametime(const Camwire_handle c_handle,
			   const float multiple);
static void cleanup(const Camwire_handle c_handle);


/*
  ----------------------------------------------------------------------
*/

int main(int argc, char *argv[])
{
    Camwire_handle *handle_array = NULL;
    Camwire_handle c_handle = NULL;
    int num_cameras, current_cam;
    long int num_iter;
    Camwire_id camid;
    int running, single_shot;
    stats_t stats;
    float shutter, framerate;

    get_command_line(argc, argv, &current_cam, &num_iter);
    
    /* Initialize the camera bus: */
    printf("\n"
	   "libdc1394 latency meter\n"
	   "(finds out how soon a captured frame becomes available).\n"
	   "\n");
    handle_array = camwire_bus_create(&num_cameras);
    if (num_cameras <= 0 || handle_array == NULL)
    {
	fprintf(stderr, "\nCould not initialize the camera bus.  "
		"There may not be a camera connected.\n");
	cleanup(NULL);
	exit(EXIT_FAILURE);
    }
    if (num_cameras == 1)
    {
	printf("Found one camera.\n");
    }
    else
    {
	printf("Found %d cameras.\nWorking with camera %d.\n",
	       num_cameras, current_cam + 1);
    }
    if (current_cam >= num_cameras)
    {
	fprintf(stderr, "\nCamera number argument %d exceeds the "
		"number of cameras found.\n", current_cam + 1);
	cleanup(NULL);
	exit(EXIT_FAILURE);
    }

    /* Initialize the camera: */
    c_handle = handle_array[current_cam];
    if (camwire_create(c_handle) != CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "\nCould not initialize camera.\n");
	cleanup(NULL);
	exit(EXIT_FAILURE);
    }

    /* Print some info: */
    if (camwire_get_identifier(c_handle, &camid) !=
	CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "\nCould not get the identifier.\n");
	cleanup(c_handle);
	exit(EXIT_FAILURE);
    }
    if (num_cameras > 1)  printf("\nCamera %d:\n", current_cam + 1);
    printf("Vendor name:       %s\n", camid.vendor);
    printf("Model name:        %s\n", camid.model);
    printf("Vendor & chip ID:  %s\n", camid.chip);
    fflush(stdout);

    /* Check basic pre-conditions: */
    camwire_get_single_shot(c_handle, &single_shot);
    if (single_shot)
    {
	fprintf(stderr, "\nCamera is set to single-shot.  "
		"Setting it to continuous.\n");
	if (camwire_set_single_shot(c_handle, 0) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr, "\nCould not unset single-shot.\n");
	    cleanup(c_handle);
	    exit(EXIT_FAILURE);
	}
    }
    camwire_get_run_stop(c_handle, &running);
    {
	fprintf(stderr, "\nCamera is stopped.  "
		"Setting it to run.\n");
	if (camwire_set_run_stop(c_handle, 1) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr, "\nCould not set camera running.\n");
	    cleanup(c_handle);
	    exit(EXIT_FAILURE);
	}
    }
    
    /* Measure and report:*/
    printf("\nCapturing %ld frames...\n", num_iter);
    measure_latency(c_handle, num_iter, &stats);
    camwire_get_shutter(c_handle, &shutter);
    camwire_get_framerate(c_handle, &framerate);
    printf("\n"
	   "shutter             %.4lg ms\n"
	   "transmission      + %.4lg ms %s\n"
	   "nominal latency   = %.4lg ms\n",
	   1e3*shutter, 1e3/framerate,
	   (shutter*framerate > 1 ? "<- PROBLEM" : ""),
	   1e3*shutter + 1e3/framerate);
    printf("\n"
	   "minimum             %.4lg ms\n"
	   "maximum             %.4lg ms\n"
	   "median latency      %.4lg ms\n"
	   "extra delay         %.4lg ms\n"
	   "measured framerate  %.4lg frames per second\n",
	   1e3*stats.min, 1e3*stats.max, 1e3*stats.median,
	   1e3*(stats.median - shutter - 1.0/framerate), stats.fps);

    /* Wrap up: */
    cleanup(c_handle);
    return(EXIT_SUCCESS);
}

/*
  ----------------------------------------------------------------------
  Checks the command line options (any option triggers a syntax message
  and exit) and gets the command line arguments if any.  Returns the
  current camera and number of iterations with default values if there
  were no arguments.
*/
static void get_command_line(int argc, char *argv[], int *curr_cam,
			     long int *n_iter)
{
    int option;
    int print_help;
    char *end_pointer;

    *curr_cam = 0;  /* Defaults.*/
    *n_iter = 100;
    print_help = 0;
    for (;;)
    {
	option = getopt(argc, argv, "h");
	if (option == -1)  break;
	else		   print_help = 1;
    }

    if (print_help)
    {
	fprintf(stderr,
		"Syntax: %s [-h | --help] "
		"[cam_number [num_iterations]]\n"
		"        Defaults to %ld iterations on camera %d.\n",
		argv[0], *n_iter, *curr_cam + 1);
	exit(EXIT_FAILURE);
    }
    
    if (optind < argc)
    {
	*curr_cam = strtol(argv[optind], &end_pointer, 10) - 1;
	if (*end_pointer != '\0')
	{
	    fprintf(stderr, "\n`%s' is not a valid argument.\n",
		    argv[optind]);
	    exit(EXIT_FAILURE);
	}
    }
    if (*curr_cam < 0)
    {
	fprintf(stderr, "Camera number %d should be positive.\n",
		*curr_cam + 1);
	exit(EXIT_FAILURE);
    }
    
    optind++;
    if (optind < argc)
    {
	*n_iter = strtol(argv[optind], &end_pointer, 10);
	if (*end_pointer != '\0')
	{
	    fprintf(stderr, "\n`%s' is not a valid argument.\n",
		    argv[optind]);
	    exit(EXIT_FAILURE);
	}
    }
    if (*n_iter <= 0)
    {
	fprintf(stderr, "Number of iterations %ld should be positive.\n",
		*n_iter);
	exit(EXIT_FAILURE);
    }
    
    optind++;
    if (optind < argc)
    {
	fprintf(stderr, "Too many command line options.\n");
	exit(EXIT_FAILURE);
    }
}

/*
  ----------------------------------------------------------------------
*/
static void measure_latency(const Camwire_handle c_handle,
			    const long int num_iter, stats_t *s)
{
    double *latency;
    struct timespec timestamp;
    double start_time, received_time, trigger_time;
    long int i, m_index;
    void *buf_ptr;
    struct timeval tv_received;

    latency = (double *) malloc(num_iter*sizeof(double));
    if (latency == NULL)
    {
	fprintf(stderr, "\nCould not allocate latency array.\n");
	cleanup(c_handle);
	exit(EXIT_FAILURE);
    }
    s->min = DBL_MAX;
    s->max = -DBL_MAX;
    start_time = trigger_time = 0.0;
    for (i = 0; i < num_iter; i++)
    {
	camwire_point_next_frame(c_handle, &buf_ptr, NULL);
	gettimeofday(&tv_received, NULL);
	camwire_get_timestamp(c_handle, &timestamp);
	camwire_unpoint_frame(c_handle);
	trigger_time = timestamp.tv_sec + 1.0e-9*timestamp.tv_nsec;
	if (start_time == 0.0)  start_time = trigger_time;
	received_time = tv_received.tv_sec + 1e-6*tv_received.tv_usec;
	latency[i] = received_time - trigger_time;
	if (latency[i] < 0.0)
	{
	    fprintf(stderr, "\nFrame %ld was received %.4lg ms before it "
		    "was captured?\n", i + 1, -1e3*latency[i]);
	    cleanup(c_handle);
	    exit(EXIT_FAILURE);
	}
	if (latency[i] < s->min)  s->min = latency[i];
	if (latency[i] > s->max)  s->max = latency[i];
    }
    qsort(latency, num_iter, sizeof(double), sort_compare);
    m_index = num_iter/2;
    if (2*m_index == num_iter)
    {
	s->median = (latency[m_index-1] + latency[m_index])/2.0;
    }
    else
    {
	s->median = latency[m_index];
    }
    if (trigger_time > start_time)
    {
	s->fps = (num_iter - 1)/(trigger_time - start_time);
    }
    else
    {
	s->fps = 0.0;
    }
    free(latency);
}

/*
  ----------------------------------------------------------------------
  Comparison function for qsort().
*/
static int sort_compare(const void *pa, const void *pb)
{
    double a, b;
    int result;

    a = *((double *) pa);
    b = *((double *) pb);
    if (a == b)      result = 0;
    else if (a < b)  result = -1;
    else             result = 1;
    return(result);
}

/*
  ----------------------------------------------------------------------
  Sleeps for multiple times one frame period, where multiple is a float.
  Keep in mind that the shortest resulting sleep is likely to be at
  least one system tick, probably two.
*/
static void wait_frametime(const Camwire_handle c_handle,
			   const float multiple)
{
    float framerate, frameperiod;
    struct timespec nap;
    
    camwire_get_framerate(c_handle, &framerate);
    frameperiod = multiple/framerate;
    nap.tv_sec = frameperiod;
    nap.tv_nsec = (frameperiod - nap.tv_sec)*1e9;
    nanosleep(&nap, NULL);
}

/*
  ----------------------------------------------------------------------
  Resets the console input to default settings, stops the camera, closes
  the display window, frees the frame buffer, and destroys the camera
  object and its bus.  Should be safe to call with null arguments.
*/
static void cleanup(const Camwire_handle c_handle)
{
    if (c_handle != NULL)
    {
	camwire_set_run_stop(c_handle, 0);
	wait_frametime(c_handle, 2.5);
	camwire_destroy(c_handle);
    }
    camwire_bus_destroy();
}
