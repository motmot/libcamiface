/***********************************************************************

    Copyright (c) Industrial Research Limited 2004

    This file is part of Camwire, a generic camera interface.

    Camwire is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of the
    License, or (at your option) any later version.

    Camwire is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Camwire; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    USA

    Title: Camera monitor

    Description:
    Provides basic access to digital camera functions via a simple
    terminal interface and display interface.  Uses the Camwire generic
    API.

    To do:
    Re-design the whole thing.  It grew organically as Camwire was
    developed and is a real mess.

***********************************************************************/


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>

#include "camwire/camwire.h"
#include "display.h"
#include "display-codes.h"


#define COMMAND_PROMPT		"> "
#define FRAMENUMBER_TOKEN	"%d"
#define DEFAULT_IMAGE_FILENAME	"cammonitor" FRAMENUMBER_TOKEN
#define DEFAULT_DISPLAY_TYPE	DISPLAY_SDL
#define DEFAULT_SAVE_NUM_IMAGES	1
#define DEFAULT_SAVE_DELAY	0.0
#define SAFE_FRAME_PERIOD	1.5
#define MAX_FILENAME		1000
#define MAX_MESSAGE		1000
#define MAX_IMAGE_HEADER	200
#define MAX_TIMESTAMP		50
#define MAX_KEY_INPUT		100
#define BUF_LOW_MARK		0.1
#define BUF_HIGH_MARK		0.9
#define MAX_SETTINGS_LINE	500
#define MAX_PIXEL_CODING_STRING	10
#define WHITESPACE		" \t\r\n"
#define SETTINGS_COMMENT	'#'


typedef struct 
{
    enum {direct, shadow} cam_access;
    enum {stopped, running} activity;
    enum {continuous, single} acqtype;
    enum {internal, external} trigger;
    enum {falling, rising} polarity;
    float shutter;
    float framerate;
    int width;
    int height;
    Camwire_pixel coding;
    int left;
    int top;
    float white_bal_blue;
    float white_bal_red;
    int num_buffers;
    long int framenumber;
    int save_num_images;
    float save_delay;
    char imagefilename[MAX_FILENAME+1];
}
Settings_t;


/* Global variables needed in various places: */
static int use_fsync = 1;
static Display_type use_display = DEFAULT_DISPLAY_TYPE;
static int display_initialized = 0;
static Display_handle d_handle = NULL;
static const char *settings_format =
        "cam_access      %d\n"
	"activity        %d\n"
	"acqtype         %d\n"
	"trigger         %d\n"
	"polarity        %d\n"
	"shutter         %g\n"
	"framerate       %g\n"
	"width           %d\n"
	"height          %d\n"
	"coding          %d\n"
	"left            %d\n"
	"top             %d\n"
        "white_bal_blue  %g\n"
        "white_bal_red   %g\n"
	"num_buffers     %d\n"
	"framenumber     %ld\n"
	"save_num_images %d\n"
	"save_delay      %g\n"
	"imagefilename   %s\n";
/* This is dreadful design, because the number and order of the fields
   in the format string above are important.  There must be exactly one
   conversion for each member of the Settings_t structure above.  It is
   used in settings_save() and update_settings(). */


/* Private prototypes: */
static int get_user_cam(const Camwire_handle *handle_array,
			const int num_cameras);
static int get_named_cam(const Camwire_handle *handle_array,
			 const int num_cameras,
			 const char *chip_id_string);
static void get_camera_settings(const Camwire_handle c_handle,
			       Settings_t *set);
static void default_noncamera_settings(Settings_t *set);
static void show_menu(const Settings_t *set);
static void show_prompt(void);
static int user_input(const Camwire_handle c_handle, const int key,
		      Settings_t *set);
static int set_shadow(const Camwire_handle c_handle);
static int set_run_stop(const Camwire_handle c_handle);
static int set_acq_type(const Camwire_handle c_handle);
static float set_shutter(const Camwire_handle c_handle);
static int set_external_trigger(const Camwire_handle c_handle);
static int set_trigger_polarity(const Camwire_handle c_handle);
static float set_framerate(const Camwire_handle c_handle);
static void set_framesize(const Camwire_handle c_handle, int *width,
			   int *height);
static void set_pixel_coding(const Camwire_handle c_handle,
			     Camwire_pixel *coding);
static void set_offset(const Camwire_handle c_handle, int *left,
		       int *top);
static void set_white_bal(const Camwire_handle c_handle, float *blue,
			  float *red);
static void set_buffers(const Camwire_handle c_handle, int *num_bufs);
static void set_image_filename(Settings_t *set);
static void set_save_delay(Settings_t *set);
static void set_save_num_images(Settings_t *set);
static int get_user_char_setting(const Camwire_handle c_handle,
				 int (*get_function)(
				     const Camwire_handle, int *),
				 char prompt_message[],
				 char true_char,
				 char false_char);
static void save_images(const Camwire_handle c_handle, Settings_t *set);
static void save_numbered_image(const Camwire_handle c_handle,
				const void *fb,
				const int image_type,
				Settings_t *set,
				FILE *logfile);
static void filename_framenumber(char *namebuffer,
				 const char *filename,
				 const int strip,
				 const long frameno);
static void manage_buffer_level(const Camwire_handle c_handle,
				FILE *logfile);
static char * pixelcoding2string(const Camwire_pixel coding);
static Camwire_pixel string2pixelcoding(const char *str);
static void settings_save_load(const Camwire_handle c_handle,
			       Settings_t *set);
static void settings_save(Settings_t *set,
			  const char *settings_filename);
static void settings_load(const Camwire_handle c_handle,
			  Settings_t *set,
			  const char *settings_filename);
static int update_settings(FILE *settingsfile, Settings_t *new);
static void check(const int scanresult, const char *tag, int *gotinput);
static char * skip_whitespace(const char *string);
static char * skip_non_whitespace(const char *string);
static void update_camera(const Camwire_handle c_handle,
			  Settings_t *new,
			  const Settings_t *set);
static void show_camwire_data(const Camwire_handle c_handle);
static void wait_frametime(const Camwire_handle c_handle,
			   const float multiple);
static void clear_stdin(void);
static void usage_message(char * const argv[]);
static int stricmp(const char *str1, const char *str2);
static void errorexit(const Camwire_handle c_handle, const int cam,
		      const char *msg);
static void cleanup(const Camwire_handle c_handle);


/*
  ----------------------------------------------------------------------
  Initializes the camera bus, gets the user to select a camera if there
  is more than one, initializes it, and sets up the display interface.
  Then enters an endless loop which displays images and reponds to user
  commands.
*/
/* Almost every camera function call returns a success/failure flag
   which should be checked, resulting in quite messy-looking code.  To
   keep things clean and simple, almost all errors therefore result in a
   call to errorexit() which prints a message, cleans up allocated
   memory, and exits with an error status. */

int main(int argc, char * const argv[])
{
    int more_options;
    int option;
    char settings_filename[MAX_FILENAME+1] = "\0";
    char chip_id_string[CAMWIRE_ID_MAX_CHARS+1] = "\0";
    Camwire_handle *handle_array = NULL;
    Camwire_handle c_handle = NULL;
    int retry;
    int bad_bus;
    int num_cameras, current_cam;
    void *capturebuffer = NULL;
    int display_return;
    int key;
    Settings_t settings;
    int runsts;
    struct timespec timestamp;
    fd_set rfds;
    struct timeval tv;

    /* Check command line options: */
    opterr = 0; 	/* Suppress error messages in getopt().*/
    use_fsync = 1;
    use_display = DEFAULT_DISPLAY_TYPE;
    more_options = 1;
    while (more_options)
    {
	option = getopt(argc, argv, "sd:e:c:");
	switch (option)
	{
	    case 's':
		use_fsync = 0;
		break;
	    case 'd':
		if (stricmp(optarg, "NONE") == 0)
		{
		    use_display = DISPLAY_NONE;
		}
		else if (stricmp(optarg, "SDL") == 0) 
		{
		    use_display = DISPLAY_SDL;
		}
		else if (stricmp(optarg, "XV") == 0) 
		{
		    use_display = DISPLAY_XV;
		}
		else
		{
		    fprintf(stderr,
			    "Did not recognise display type %s\n",
			    optarg);
		    usage_message(argv);
		    return(EXIT_FAILURE);
		}
		break;
	    case 'e':
		if (optarg != NULL)
		{
		    strncpy(settings_filename, optarg, MAX_FILENAME);
		}
		else
		{
		    usage_message(argv);
		}
		break;
	    case 'c':
		if (optarg != NULL)
		{
		    strncpy(chip_id_string, optarg,
			    CAMWIRE_ID_MAX_CHARS);
		}
		else
		{
		    usage_message(argv);
		}
		break;
	    case ':':
		fprintf(stderr, "Expected a parameter after command "
			"line option -%c\n", optopt);
		usage_message(argv);
		return(EXIT_FAILURE);
		break;
	    case -1:
		more_options = 0;
		break;
	    case '?':
		fprintf(stderr,
			"Did not recognise command line option %c\n",
			optopt);
		usage_message(argv);
		return(EXIT_FAILURE);
		break;
	    default:
		fprintf(stderr,
			"Error parsing command line arguments.\n");
		usage_message(argv);
		return(EXIT_FAILURE);
		break;
	}
    }

    /* Initialize the camera bus:*/
    printf("\nCamera monitor\n\n");
    handle_array = camwire_bus_create(&num_cameras);
    if (num_cameras < 0 || handle_array == NULL)
    { 	/* Could be the known kernel 1394 cycle master bug. */
	printf("Error initializing the bus: trying reset");
	retry = 1;
	bad_bus = 1;
	while (retry <= 5 && bad_bus)
	{
	    printf(" %d", retry);
	    fflush(stdout);
	    camwire_bus_reset();
	    sleep(1);
	    handle_array = camwire_bus_create(&num_cameras);
	    if (num_cameras >= 0 && handle_array != NULL)  bad_bus = 0;
	    retry++;
	}
	if (bad_bus)
	{
	    errorexit(NULL, -1,
		      "Could not initialize the camera bus.  "
		      "There may not be a camera connected.");
	}
	else
	{
	    printf("\n\n");
	    fflush(stdout);
	}
    }
    if (num_cameras == 0)
    {
	errorexit(NULL, -1, "Could not find a camera.");
    }
    if (num_cameras == 1)  printf("Found one camera.\n");
    else                   printf("Found %d cameras.\n\n", num_cameras);
    fflush(stdout);

    /* Identify the camera to use: */
    if (chip_id_string[0] != '\0')
    {
	/* Find the camera named on the command line: */
	current_cam =
	    get_named_cam(handle_array, num_cameras, chip_id_string);
    }
    else
    {
	/* Ask user which camera to use:*/
	current_cam = get_user_cam(handle_array, num_cameras);
    }
    
    /* Initialize the camera and our local copy of its settings: */
    c_handle = handle_array[current_cam];
    if (camwire_create(c_handle) != CAMWIRE_SUCCESS)
    {
	errorexit(NULL, current_cam, "Could not initialize camera.");
    }
    get_camera_settings(c_handle, &settings);
    default_noncamera_settings(&settings);

    /* Set up display interface:*/
    if (use_display != DISPLAY_NONE)
    {
	d_handle = display_create(use_display, settings.width,
				  settings.height, settings.coding);
	if (d_handle == NULL)
	{
	    errorexit(c_handle, -1, "Could not set up display.");
	}
	display_initialized = 1;
    }

    /* Auto-load settings if a command line filename was given: */
    if (settings_filename[0] != '\0')
    {
	settings_load(c_handle, &settings, settings_filename);
    }

    /* Main loop, which gets and displays frames, and responds to user
       input:*/
    show_menu(&settings);
    show_prompt();

    for (;;)
    {
	/* Get and display the next frame:*/
	if (settings.activity == running)
	{ 	/* Avoid getting blocked if not running.*/
	    if (camwire_point_next_frame(c_handle, &capturebuffer,
					 NULL) != CAMWIRE_SUCCESS)
		errorexit(c_handle, current_cam,
			  "Could not point to the next frame.");
	    if (use_display != DISPLAY_NONE)
	    {
		display_return = display_frame(d_handle, capturebuffer);
		if (display_return == DISPLAY_FAILURE)
		{
		    fprintf(stderr, "Display error.\n");
		    cleanup(c_handle);
		    return(EXIT_FAILURE);
		}
		else if (display_return == DISPLAY_QUIT_REQ)
		{
		    cleanup(c_handle);
		    return(EXIT_SUCCESS);
		}
	    }
	    camwire_unpoint_frame(c_handle);
	    manage_buffer_level(c_handle, NULL);
		
	    if (camwire_get_run_stop(c_handle, &runsts) !=
		CAMWIRE_SUCCESS)
	    {
		fprintf(stderr, "Could not get activity status.\n");
	    }
	    if (runsts != 0)  settings.activity = running;
	    else              settings.activity = stopped;
	    
	    if (settings.acqtype == single)
	    {
		printf("\nCaptured a single shot.\n");
		fflush(stdout);
		if (settings.activity != stopped)
		{ 	/* Single-shot auto-stops camera. */
		    fprintf(stderr, "Single-shot should have stopped "
			    "camera?\n");
		}
		else
		{
		    if(camwire_get_timestamp(c_handle, &timestamp) !=
		       CAMWIRE_SUCCESS)
		    {
			fprintf(stderr, "Could not get a timestamp.\n");
		    }
		    printf("Timestamp: %lf\n",
			   timestamp.tv_sec + 1.0e-9*timestamp.tv_nsec);
		    fflush(stdout);
		
		    if (camwire_get_framenumber(c_handle,
					    &settings.framenumber)
			!= CAMWIRE_SUCCESS)
		    {
			fprintf(stderr,
				"Could not get a frame number.\n");
		    }
		    show_menu(&settings);
		    show_prompt();
		}
	    }
	}

	/* Simple user interaction:*/
	if (settings.activity == running &&
	    settings.acqtype == continuous)
	{
	    /* Check stdin (fd 0) to see if it has input: */
	    FD_ZERO(&rfds);
	    FD_SET(0, &rfds);
	    tv.tv_sec = 0; 	/* Poll, don't wait.*/
	    tv.tv_usec = 0;

	    if (select(1, &rfds, NULL, NULL, &tv))
	    {
		key = getchar();
	    }
	    else
	    {
		key = EOF;
	    }
	}
	else
	{
	    key = getchar(); 	/* Wait here for input.*/
	}
	if (key != EOF)
	{
	    if (key != '\n')
	    {
		if (!user_input(c_handle, key, &settings))
		{ 	/* User request to quit.*/
		    clear_stdin();
		    cleanup(c_handle);
		    return(EXIT_SUCCESS);
		}
		clear_stdin(); 	/* Swallow trailers and NL.*/
		if (camwire_get_framenumber(c_handle,
					    &settings.framenumber) !=
		    CAMWIRE_SUCCESS)
		{
		    fprintf(stderr, "Could not get a frame number.\n");
		}
		show_menu(&settings);
	    }
	    show_prompt();
	}
    }/*for*/
}

/*
  ----------------------------------------------------------------------
  Returns the camera to work with, which is 0 if there is only one
  camera or the camera number chosen interactively by the user if there
  is more than one camera.  Displays the camera's identifier data and
  creates it as well.
*/
static int get_user_cam(const Camwire_handle *handle_array,
			const int num_cameras)
{
    int c, current_cam;
    int got_one;
    Camwire_id camid;

    for (c = 0; c < num_cameras; c++)
    {
	if (camwire_get_identifier(handle_array[c], &camid) !=
	    CAMWIRE_SUCCESS)
	{
	    errorexit(NULL, c, "Could not get the identifier.");
	}
	if (num_cameras > 1)  printf("\nCamera %d:\n", c + 1);
	printf("Vendor name:       %s\n", camid.vendor);
	printf("Model name:        %s\n", camid.model);
	printf("Vendor & chip ID:  %s\n", camid.chip);
	fflush(stdout);
    }

    if (num_cameras == 1)
    {
	current_cam = 0;
    }
    else
    {
	got_one = 0;
	while (!got_one)
	{
	    current_cam = -1;
	    while (current_cam < 0 || current_cam > num_cameras)
	    {
		printf("\nSelect a camera number "
		       "(from 1 to %d, 0 quits): ", num_cameras);
		fflush(stdout);
		scanf("%d", &current_cam);
		clear_stdin();
	    }
	    current_cam--;
	    if (current_cam < 0) 
	    {
		camwire_bus_destroy();
		putchar('\n');
		fflush(stdout);
		exit(EXIT_SUCCESS); 	/* Normal exit.*/
	    }
	    else
	    {
		got_one = 1;
	    }
	}
    }
    return(current_cam);
}

/*
  ----------------------------------------------------------------------
  Returns the camera to work with, which is the camera number with
  matching chip identifier string.  Displays the camera's identifier
  data and creates it as well.
*/
static int get_named_cam(const Camwire_handle *handle_array,
			 const int num_cameras,
			 const char *chip_id_string)
{
    int c, current_cam;
    Camwire_id camid;

    current_cam = -1;;
    for (c = 0; c < num_cameras; c++)
    {
	if (camwire_get_identifier(handle_array[c], &camid) !=
	    CAMWIRE_SUCCESS)
	{
	    errorexit(NULL, c, "Could not get the identifier.");
	}
	if (strncmp(camid.chip, chip_id_string, CAMWIRE_ID_MAX_CHARS) ==
	    0)
	{
	    current_cam = c;
	    break;
	}
    }

    if (current_cam < 0)
    {
	printf("Could not find camera with ID %s.\n", chip_id_string);
	fflush(stdout);
	errorexit(NULL, current_cam,
		  "Try running without specifying an ID.");
    }

    printf("Vendor name:       %s\n", camid.vendor);
    printf("Model name:        %s\n", camid.model);
    printf("Vendor & chip ID:  %s\n", camid.chip);
    fflush(stdout);
    return(current_cam);
}

/*
  ----------------------------------------------------------------------
  Reads the camera's parameters and default values into the settings
  structure.
*/
static void get_camera_settings(const Camwire_handle c_handle,
			       Settings_t *set)
{
    int settingsval;
    int camwire_result;

    camwire_get_stateshadow(c_handle, &settingsval);
    if (settingsval)  set->cam_access = shadow;
    else              set->cam_access = direct;

    camwire_get_run_stop(c_handle, &settingsval);
    if (settingsval)  set->activity = running;
    else              set->activity = stopped;

    camwire_get_single_shot(c_handle, &settingsval);
    if (settingsval)  set->acqtype = single;
    else              set->acqtype = continuous;

    camwire_result = camwire_get_white_balance(c_handle, &set->white_bal_blue,
					       &set->white_bal_red);
    if (camwire_result != CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get camera white balance: Camera may "
		"not support it.\n");
	set->white_bal_blue = set->white_bal_red = 0;
    }

    camwire_result = camwire_get_trigger_source(c_handle, &settingsval);
    if (camwire_result != CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get camera trigger source: Camera may "
		"not support it.\n");
	settingsval = 0;
    }
    if (settingsval)  set->trigger = external;
    else              set->trigger = internal;

    camwire_result = camwire_get_trigger_polarity(c_handle, &settingsval);
    if (camwire_result != CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get camera trigger polarity: Camera may "
		"not support it.\n");
	settingsval = 0;
    }
    if (settingsval)  set->polarity = rising;
    else              set->polarity = falling;

    camwire_result = camwire_get_shutter(c_handle, &set->shutter);
    if (camwire_result != CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get camera shutter: Camera may "
		"not support it.\n");
	set->shutter = 0;
    }

    camwire_get_framerate(c_handle, &set->framerate);

    camwire_get_frame_size(c_handle, &set->width, &set->height);

    camwire_get_pixel_coding(c_handle, &set->coding);

    camwire_get_frame_offset(c_handle, &set->left, &set->top);

    camwire_get_num_framebuffers(c_handle, &set->num_buffers);

    camwire_get_framenumber(c_handle, &set->framenumber);
}

/*
  ----------------------------------------------------------------------
  Sets default values to the non-camera paramenters in the settings
  structure.
*/
static void default_noncamera_settings(Settings_t *set)
{
    set->save_delay = DEFAULT_SAVE_DELAY;

    set->save_num_images = DEFAULT_SAVE_NUM_IMAGES;
    
    strncpy(set->imagefilename, DEFAULT_IMAGE_FILENAME, MAX_FILENAME);
    set->imagefilename[MAX_FILENAME] = '\0';
}

/*
  ----------------------------------------------------------------------
  Prints the menu of available commands as well as the current
  settings.
*/
static void show_menu(const Settings_t *set)
{
    char *c1, *c2, *c3, *c4;
    static char leftcurr[] = "[", rightcurr[] = "]", nothing[] = "";
    char namebuffer[MAX_FILENAME+1];
    
    printf("\nCommands:  "
	   "[H]otkey         "
	   "%spossible_settings%s %scurrent_setting%s\n",
	   nothing, nothing, leftcurr, rightcurr);

    /* Activity:*/
    if (set->activity == stopped)
    {
	c1 = leftcurr;  c2 = rightcurr;
	c3 = nothing;  c4 = nothing;
    }
    else
    {
	c1 = nothing;  c2 = nothing;
	c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[R]un/stop camera           %sstopped%s %srunning%s\n",
	   c1, c2, c3, c4);

    /* Acquisition type:*/
    if (set->acqtype == continuous)
    {
	c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
	c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[A]quisition                "
	   "%scontinuous%s %ssingle-shot%s\n",
	   c1, c2, c3, c4);

    /* Trigger:*/
    if (set->trigger == internal)
    {
	c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
	c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("[T]rigger source            %sinternal%s %sexternal%s\n",
	   c1, c2, c3, c4);

    /* Polarity:*/
    if (set->polarity == falling)
    {
	c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
	c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("trigger [P]olarity          %sfalling%s %srising%s\n",
	   c1, c2, c3, c4);

    /* Integration time:*/
    printf("[I]ntegration time          %s%g s%s\n",
	   leftcurr, set->shutter, rightcurr);

    /* Frame rate:*/
    printf("[F]rame rate                %s%g fps%s\n",
	   leftcurr, set->framerate, rightcurr);

    /* Image size:*/
    printf("frame si[Z]e         "
	   "       %sw %d pixels%s %sh %d pixels%s\n",
	   leftcurr, set->width, rightcurr,
	   leftcurr, set->height, rightcurr);

    /* Pixel Coding:*/
    printf("pixel [C]oding       "
	   "       %s%s%s\n",
	   leftcurr, pixelcoding2string(set->coding), rightcurr);

    /* Image offset:*/
    printf("frame [O]ffset       "
	   "       %sleft %d pixels%s %stop %d pixels%s\n",
	   leftcurr, set->left, rightcurr,
	   leftcurr, set->top, rightcurr);

    /* White balance:*/
    printf("[W]hite balance      "
	   "       %sblue gain %g%s %sred gain %g%s\n",
	   leftcurr, set->white_bal_blue, rightcurr,
	   leftcurr, set->white_bal_red, rightcurr);

    /* Frame Buffers:*/
    printf("frame [B]uffers             %s%d%s\n",
	   leftcurr, set->num_buffers, rightcurr);

    /* Shadow camera:*/
    if (set->cam_access == direct)
    {
	c1 = leftcurr;  c2 = rightcurr;  c3 = nothing;  c4 = nothing;
    }
    else
    {
	c1 = nothing;  c2 = nothing;  c3 = leftcurr;  c4 = rightcurr;
    }
    printf("sha[D]ow camera             %sdirect%s %sshadow%s\n",
	   c1, c2, c3, c4);

    /* Image filename:*/
    filename_framenumber(namebuffer, set->imagefilename, 0,
			 set->framenumber + 1);
    printf("file[N]ame for save         %s%s%s\n",
	   leftcurr, namebuffer, rightcurr);

    /* Delay before saving:*/
    printf("de[L]ay before saving       %s%g s%s\n",
	   leftcurr, set->save_delay, rightcurr);

    /* Number of images to save:*/
    printf("[M]ultiple images to save   %s%d%s\n",
	   leftcurr, set->save_num_images, rightcurr);

    /* Save image:*/
    if (set->save_num_images == 1)
    {
	printf("[S]ave image to file\n");
    }
    else if (set->save_num_images > 1)
    {
	printf("[S]ave images to files\n");
    }
    /* else do not print save item.*/

    /* Settings save or load:*/
	printf("s[E]ttings save or load\n");

    /* Show Camwire data:*/
	printf("s[H]ow camwire data\n");
    
    /* Quit:*/
    printf("[Q]uit camera monitor\n");
    fflush(stdout);
}

/*
  ----------------------------------------------------------------------
  Prints the command line prompt.
*/
static void show_prompt(void)
{
    printf(COMMAND_PROMPT);
    fflush(stdout);
}

/*
  ----------------------------------------------------------------------
  Does something in response to the user input.
*/
static int user_input(const Camwire_handle c_handle, const int key,
		      Settings_t *set)
{
    int oldwidth, oldheight, oldcoding;

    switch(tolower(key))
    {
	case 'r': 	/* Run-stop toggle:*/
	    set->activity = set_run_stop(c_handle);
	    break;
	case 'a': 	/* Acquisition type toggle:*/
	    set->acqtype = set_acq_type(c_handle);
	    break;
	case 't': 	/* Trigger source toggle:*/
	    set->trigger = set_external_trigger(c_handle);
	    break;
	case 'p': 	/* Trigger polarity toggle:*/
	    set->polarity = set_trigger_polarity(c_handle);
	    break;
	case 'i': 	/* Integration time:*/
	    set->shutter = set_shutter(c_handle);
	    break;
	case 'f': 	/* Frame rate:*/
	    set->framerate = set_framerate(c_handle);
	    break;
	case 'z': 	/* Frame siZe:*/
	    oldwidth = set->width;
	    oldheight = set->height;
	    set_framesize(c_handle, &set->width, &set->height);
	    if (use_display != DISPLAY_NONE)
	    {
		if (display_resize(d_handle, set->width, set->height) !=
		    DISPLAY_SUCCESS)
		{ 	/* Can't display it for some reason.*/
		    set->width = oldwidth;
		    set->height = oldheight;
		    camwire_set_frame_size(c_handle, set->width,
					   set->height);
		}
	    }
	    break;
	case 'c': 	/* Pixel Coding:*/
	    oldcoding = set->coding;
	    set_pixel_coding(c_handle, &set->coding);
	    if (use_display != DISPLAY_NONE)
	    {
		if (display_coding(d_handle, set->coding) !=
		    DISPLAY_SUCCESS)
		{ 	/* Can't display it.*/
		    set->coding = oldcoding;
		    camwire_set_pixel_coding(c_handle, set->coding);
		}
	    }
	    break;
	case 'o': 	/* Frame Offset:*/
	    set_offset(c_handle, &set->left, &set->top);
	    break;
	case 'w': 	/* White balance:*/
	    set_white_bal(c_handle, &set->white_bal_blue,
			  &set->white_bal_red);
	    break;
	case 'b': 	/* Frame Buffers:*/
	    set_buffers(c_handle, &set->num_buffers);
	    break;
	case 'd': 	/* ShaDow camera:*/
	    set->cam_access = set_shadow(c_handle);
	    break;
	case 'n': 	/* Change image filename:*/
	    set_image_filename(set);
	    break;
	case 'l': 	/* Delay before saving:*/
	    set_save_delay(set);
	    break;
	case 'm': 	/* Number of images to save:*/
	    set_save_num_images(set);
	    break;
	case 's': 	/* Save images to file:*/
	    save_images(c_handle, set);
	    break;
	case 'e':
	    settings_save_load(c_handle, set);
	    break;
	case 'h':
	    show_camwire_data(c_handle);
	    break;
	case 'q': 	/* Quit:*/
	    return(0);
	    break;
	default:
	    break;
    } /*switch*/
    return(1);
}

/*
  ----------------------------------------------------------------------
  Returns the camera shadow flag obtained from the user, after setting
  the camera accordingly.
*/
static int set_shadow(const Camwire_handle c_handle)
{
    int doshadow;

    doshadow =
	get_user_char_setting(c_handle, camwire_get_stateshadow,
			      "Shadow/Direct camera access",
			      's', 'd');

    if (doshadow)
    {
	if (camwire_set_stateshadow(c_handle, 1) !=
	    CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		    "Could not set camera access to shadow.\n");
	}
    }
    else
    {
	if (camwire_set_stateshadow(c_handle, 0) !=
	    CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		"Could not set camera access to direct.\n");
	}
    }

    if (camwire_get_stateshadow(c_handle, &doshadow) !=
	CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get camera access method.\n");
    }
    if (doshadow)  return(shadow);
    else	   return(direct);
}

/*
  ----------------------------------------------------------------------
  Returns the activity status obtained from the user, after setting the
  camera accordingly.
*/
static int set_run_stop(const Camwire_handle c_handle)
{
    int run;
    int num_buffers;

    run = get_user_char_setting(c_handle, camwire_get_run_stop,
				"Run/Stop camera", 'r', 's');

    if (run)
    {
	if (camwire_set_run_stop(c_handle, 1) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr, "Could not set camera to running.\n");
	}
    }
    else
    {
	if (camwire_set_run_stop(c_handle, 0) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr, "Could not set camera to stopped.\n");
	}
	wait_frametime(c_handle, SAFE_FRAME_PERIOD);
	camwire_get_num_framebuffers(c_handle, &num_buffers);
	camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
    }
    
    /* Note that we do not bother to check the camera run/stop status
       again here because it auto-clears when single-shot is set:*/
    if (run)  return(running);
    else      return(stopped);
}

/*
  ----------------------------------------------------------------------
  Returns the acquisition type obtained from the user, after setting
  the camera accordingly.
*/
static int set_acq_type(const Camwire_handle c_handle)
{
    int singleshot;
    int num_buffers;

    singleshot =
	get_user_char_setting(c_handle, camwire_get_single_shot,
			      "Continuous/Single-shot acquisition",
			      's', 'c');
    
    if (singleshot)
    {
	camwire_get_num_framebuffers(c_handle, &num_buffers);
	camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
	if (camwire_set_single_shot(c_handle, 1) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		"Could not set camera to single-shot acquisition.\n");
	}
    }
    else
    {
	if (camwire_set_single_shot(c_handle, 0) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		"Could not set camera to continuous acquisition.\n");
	}
    }

    if (camwire_get_single_shot(c_handle, &singleshot) !=
	CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get single-shot status.\n");
    }
    if (singleshot)  return(single);
    else             return(continuous);
}

/*
  ----------------------------------------------------------------------
  Returns a shutter speed (exposure time) obtained from the user, after
  setting the camera accordingly.
*/
static float set_shutter(const Camwire_handle c_handle)
{
    int gotinput;
    float shutter;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Integration time (seconds): ");
	fflush(stdout);
	if (scanf("%f", &shutter) == 1)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid floating point number.\n");
	    clear_stdin();
	}
    }
    if (camwire_set_shutter(c_handle, shutter) != CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the camera shutter.\n");
    if (camwire_get_shutter(c_handle, &shutter) != CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not confirm the camera shutter.\n");
    return(shutter);
}

/*
  ----------------------------------------------------------------------
  Returns the trigger source obtained from the user, after setting the
  camera accordingly.
*/
static int set_external_trigger(const Camwire_handle c_handle)
{
    int exttrig;

    exttrig = get_user_char_setting(c_handle,
				    camwire_get_trigger_source,
				    "Internal/External trigger",
				    'e', 'i');

    if (exttrig)
    {
	if (camwire_set_trigger_source(c_handle, 1) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		    "Could not set trigger source to external.\n");
	}
    }
    else
    {
	if (camwire_set_trigger_source(c_handle, 0) != CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		"Could not set trigger source to internal.\n");
	}
    }

    if (camwire_get_trigger_source(c_handle, &exttrig) !=
	CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get trigger source.\n");
    }
    if (exttrig)  return(external);
    else	  return(internal);
}

/*
  ----------------------------------------------------------------------
  Returns the trigger polarity obtained from the user, after setting
  the camera accordingly.
*/
static int set_trigger_polarity(const Camwire_handle c_handle)
{
    int risingpol;

    risingpol =
	get_user_char_setting(c_handle, camwire_get_trigger_polarity,
			      "Rising/Falling trigger polarity",
			      'r', 'f');

    if (risingpol)
    {
	if (camwire_set_trigger_polarity(c_handle, 1) !=
	    CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		    "Could not set trigger polarity to rising edge.\n");
	}
    }
    else
    {
	if (camwire_set_trigger_polarity(c_handle, 0) !=
	    CAMWIRE_SUCCESS)
	{
	    fprintf(stderr,
		"Could not set trigger polarity to falling edge.\n");
	}
    }

    if (camwire_get_trigger_polarity(c_handle, &risingpol) !=
	CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get trigger polarity.\n");
    }
    if (risingpol)  return(rising);
    else	    return(falling);
}

/*
  ----------------------------------------------------------------------
  Returns a frame rate (frames per second) obtained from the user,
  after setting the camera accordingly.
*/
static float set_framerate(const Camwire_handle c_handle)
{
    int gotinput;
    float framerate;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Frame rate (frames per second): ");
	fflush(stdout);
	if (scanf("%f", &framerate) == 1)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid floating point number.\n");
	    clear_stdin();
	}
    }
    if (camwire_set_framerate(c_handle, framerate) != CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the camera framerate.\n");
    if (camwire_get_framerate(c_handle, &framerate) != CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not confirm the camera framerate.\n");
    return(framerate);
}

/*
  ----------------------------------------------------------------------
  Returns the frame size obtained from the user, after setting the
  camera accordingly.
*/
static void set_framesize(const Camwire_handle c_handle, int *width,
			   int *height)
{
    int gotinput;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Frame size (width height): ");
	fflush(stdout);
	if (scanf("%d %d", width, height) == 2)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid number.\n");
	    clear_stdin();
	}
    }
    if (camwire_set_frame_size(c_handle, *width, *height) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the camera frame size.\n");
    if (camwire_get_frame_size(c_handle, width, height) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr,
		"Could not confirm the camera frame size.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the pixel coding obtained from the user, after setting
  the camera accordingly.
*/
static void set_pixel_coding(const Camwire_handle c_handle,
			     Camwire_pixel *coding)
{
    int gotinput;
    char pix_coding[MAX_PIXEL_CODING_STRING+1];
    char formatstring[20];

    /* Limit the input width: */
    sprintf(formatstring, "%%%ds", MAX_PIXEL_CODING_STRING);
      
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Pixel coding.  Type one of:\n"
	       "MONO8\n"
	       "YUV411\n"
	       "YUV422\n"
	       "YUV444\n"
	       "RGB8\n"
	       "MONO16\n"
	       "RGB16\n"
	       "MONO16S\n"
	       "RGB16S\n"
	       "RAW8\n"
	       "RAW16\n"
	       ": ");
	fflush(stdout);
	if (scanf(formatstring, pix_coding) == 1)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid coding string.\n");
	    clear_stdin();
	}
    }

    *coding = string2pixelcoding(pix_coding);
    if (camwire_set_pixel_coding(c_handle, *coding) != CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the camera's pixel coding.\n");
    if (camwire_get_pixel_coding(c_handle, coding) != CAMWIRE_SUCCESS)
	fprintf(stderr,
		"Could not confirm the camera's pixel coding.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the frame offset obtained from the user, after setting
  the camera accordingly.
*/
static void set_offset(const Camwire_handle c_handle, int *left,
		       int *top)
{
    int gotinput;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Frame offset (left top): ");
	fflush(stdout);
	if (scanf("%d %d", left, top) == 2)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid number.\n");
	    clear_stdin();
	}
    }
    if (camwire_set_frame_offset(c_handle, *left, *top) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the camera frame offset.\n");
    if (camwire_get_frame_offset(c_handle, left, top) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not confirm the camera frame offset.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the white balance obtained from the user, after setting
  the camera accordingly.
*/
static void set_white_bal(const Camwire_handle c_handle, float *blue,
			  float *red)
{
    int gotinput;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("White balance (blue red): ");
	fflush(stdout);
	if (scanf("%f %f", blue, red) == 2)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid number.\n");
	    clear_stdin();
	}
    }
    if (camwire_set_white_balance(c_handle, *blue, *red) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the camera white balance.\n");
    if (camwire_get_white_balance(c_handle, blue, red) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr,
		"Could not confirm the camera white balance.\n");
}

/*
  ----------------------------------------------------------------------
  Returns the number of frame buffers obtained from the user, after
  setting the camera accordingly.
*/
static void set_buffers(const Camwire_handle c_handle, int *num_bufs)
{
    int gotinput;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Number of frame buffers: ");
	fflush(stdout);
	if (scanf("%d", num_bufs) == 1)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid number.\n");
	    clear_stdin();
	}
    }
    if (camwire_set_num_framebuffers(c_handle, *num_bufs) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr, "Could not set the number of frame buffers.\n");
    if (camwire_get_num_framebuffers(c_handle, num_bufs) !=
	CAMWIRE_SUCCESS)
	fprintf(stderr,
		"Could not confirm the number of frame buffers.\n");
}

/*
  ----------------------------------------------------------------------
  Changes the name of the root image filename.
*/
static void set_image_filename(Settings_t *set)
{
    char formatstring[20];
    
    sprintf(formatstring, "%%%ds", MAX_FILENAME); 	/* Limit the
							 * input field
							 * width.*/
    printf("\nImage root filename (currently \"%s\"): ",
	   set->imagefilename);
    fflush(stdout);
    scanf(formatstring, set->imagefilename);
}

/*
  ----------------------------------------------------------------------
  Sets the delay before image saving starts.
*/
static void set_save_delay(Settings_t *set)
{
    int gotinput;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Delay in seconds before saving (was %g): ",
	       set->save_delay);
	fflush(stdout);
	if (scanf("%f", &set->save_delay) == 1)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid number.\n");
	    clear_stdin();
	}
    }
}

/*
  ----------------------------------------------------------------------
  Changes the number of frames saved at a time.
*/
static void set_save_num_images(Settings_t *set)
{
    int gotinput;
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Number of frames to save at a time (was %d): ",
	       set->save_num_images);
	fflush(stdout);
	if (scanf("%d", &set->save_num_images) == 1)
	{
	    gotinput = 1;
	}
	else
	{
	    fprintf(stderr, "Invalid number.\n");
	    clear_stdin();
	}
    }
}

/*
  ----------------------------------------------------------------------
  Returns a setting by prompting the user for a single character until
  one of the wanted characters is input.  The user's terminating newline
  is left in the stdin buffer.
*/
static int get_user_char_setting(const Camwire_handle c_handle,
				 int (*get_function)(
				     const Camwire_handle, int *),
				 char prompt_message[],
				 char true_char,
				 char false_char)
{
    int setting, gotinput;
    char formatstring[20];
    char userinput[MAX_KEY_INPUT+1];
    char key;
    
    if (get_function(c_handle, &setting) != CAMWIRE_SUCCESS)
    {
	fprintf(stderr, "Could not get current camera setting.\n");
    }
    true_char = toupper(true_char);
    false_char = toupper(false_char);
    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT); 	/* Limit the
							 * input field
							 * width.*/
    
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("%s (currently %c): ", prompt_message,
	       setting ? true_char : false_char);
	fflush(stdout);
	scanf(formatstring, userinput); 	/* Safe input.*/
	key = toupper(userinput[0]);
	if (key == true_char)
	{
	    setting = 1;
	    gotinput = 1;
	}
	else if (key == false_char)
	{
	    setting = 0;
	    gotinput = 1;
	}
	else
	{
	    printf("Type either %c or %c.\n", true_char, false_char);
	    fflush(stdout);
	    clear_stdin();
	}
    }
    return(setting);
}

/*
  ----------------------------------------------------------------------
  Captures, displays and saves the next frame(s) in image file(s), with
  optional delay.  Writes frame number, time stamp and buffer level in a
  log file for each frame.  Prints an error message if a file cannot be
  opened.  Tries to ensure that the frame buffers does not overflow by
  flushing frames when the buffer gets too full.
*/
static void save_images(const Camwire_handle c_handle, Settings_t *set)
{
    int depth;
    int now_running, now_single;
    float framerate, frameperiod, waitperiod;
    struct timespec nap;
    FILE *logfile;
    char logfile_name[MAX_FILENAME+1];
    char errormessage[MAX_MESSAGE+1];
    char *nameptr;
    int num_buffers, count;
    int image_type;
    void *fb;
    int display_saved_frames;

    /* Skip unsupported pixel codings: */
    if (set->coding == CAMWIRE_PIXEL_YUV411)
    {
	fprintf(stderr, "Don't know how to save YUV411.\n");
	return;
    }
    camwire_pixel_depth(set->coding, &depth);
    
    /* Find out the current status and update our record: */
    camwire_get_run_stop(c_handle, &now_running);
    if (now_running)  set->activity = running;
    else              set->activity = stopped;
    camwire_get_single_shot(c_handle, &now_single);
    if (now_single)  set->acqtype = single;
    else             set->acqtype = continuous;
    camwire_get_framerate(c_handle, &framerate);
    camwire_get_num_framebuffers(c_handle, &num_buffers);

    /* If camera is stopped, or if delay is required, stop, wait,
       flush, and restart: */
    frameperiod = 1.0/framerate;
    if (!now_running || set->save_delay > frameperiod)
    {
	if (now_running)
	{
	    now_running = 0;
	    camwire_set_run_stop(c_handle, now_running);
	}
	if (set->save_delay > frameperiod)
	{
	    waitperiod = set->save_delay - frameperiod;
	    nap.tv_sec = waitperiod;
	    nap.tv_nsec = (waitperiod - nap.tv_sec)*1e9;
	    nanosleep(&nap, NULL);
	}
	wait_frametime(c_handle, SAFE_FRAME_PERIOD);
	camwire_flush_framebuffers(c_handle, num_buffers, NULL, NULL);
	if (set->activity == running)
	{
	    now_running = 1;
	    camwire_set_run_stop(c_handle, now_running);
	}
	/* Entry status is now restored. */
    }

    /* Return now if no images are needed: */
    if (set->save_num_images < 1)  return;

    /* If the camera was already running, grab images on the fly.  If
       the camera is stopped, use single-shot for single images and
       continuous running for multiple images: */
    if (set->activity == stopped)
    {
	if (set->save_num_images == 1 && set->acqtype == continuous)
	{
	    now_single = 1;
	    camwire_set_single_shot(c_handle, now_single);
	}
	else if (set->save_num_images > 1 && set->acqtype == single)
	{
	    now_single = 0;
	    camwire_set_single_shot(c_handle, now_single);
	}
	now_running = 1;
	camwire_set_run_stop(c_handle, now_running);
    }
    /* Camera is now running. */

    /* Decide on type of image to save: */
    switch (depth)
    {
	case 8:
	case 16:
	    image_type = 5; 	/* PGM.*/
	    break;
	case 24:
	case 48:
	    image_type = 6; 	/* PPM.*/
	    break;
	case 12:
	default:
	    image_type = 0; 	/* YUV411?*/
	    break;
    }

    /* Set up image saving: */
    if (set->save_num_images == 1)
    {
	printf("Saving a %s image.\n",
	       (image_type == 6 ? "PPM" : "PGM"));
    }
    else
    {
	printf("Saving %d %s images at %g fps.\n",
	       set->save_num_images,
	       (image_type == 6 ? "PPM" : "PGM"),
	       framerate);
    }
    fflush(stdout);

    display_saved_frames = 0;
    if (use_display != DISPLAY_NONE)
    {
	fb = (void *) malloc((size_t) set->width*set->height*depth/8);
	if (fb != NULL)
	{
	    display_saved_frames = 1;
	}
	else
	{
	    fprintf(stderr, "Could not allocate display buffer.\n"
		    "Display disabled.\n");
	}
    }
    
    filename_framenumber(logfile_name, set->imagefilename, 1, 0);
    nameptr = &logfile_name[strlen(logfile_name)-4];
    if (strncmp(nameptr, ".ppm", 4) == 0 ||
	strncmp(nameptr, ".pgm", 4) == 0)
    {
	*nameptr = '\0'; 	/* Truncate ".ppm" or ".pgm".*/
    }
    strcat(logfile_name, ".log");
    logfile = fopen(logfile_name, "w");
    if (logfile == NULL)
    {
	snprintf(errormessage, MAX_MESSAGE, "Can't create '%s'",
		 logfile_name);
	perror(errormessage);
    }

    /* Grab, display and save images: */
    for (count = 1; count <= set->save_num_images; count++)
    {
	if (display_saved_frames)
	{
	    camwire_copy_next_frame(c_handle, fb, NULL);
	    display_frame(d_handle, fb);
	    save_numbered_image(c_handle, fb, image_type, set, logfile);
	}
	else
	{
	    camwire_point_next_frame(c_handle, &fb, NULL);
	    save_numbered_image(c_handle, fb, image_type, set, logfile);
	    camwire_unpoint_frame(c_handle);
	}
	manage_buffer_level(c_handle, logfile);
    }

    fclose(logfile);
    if (display_saved_frames)  free(fb);

    /* Single-shot acquisition automatically stops the camera: */
    if (now_single)
    {
	camwire_get_run_stop(c_handle, &now_running);
	if (now_running != 0)
	{ 	/* Double check.*/
	    errorexit(c_handle, -1,
		      "Camera should have stopped after single-shot.");
	}
    }

    /* Restore entry settings: */
    if (set->activity == stopped)
    {
	if (set->save_num_images == 1 && set->acqtype == continuous)
	{
	    now_single = 0;
	    camwire_set_single_shot(c_handle, now_single);
	}
	else if (set->save_num_images > 1)
	{
	    now_running = 0;
	    camwire_set_run_stop(c_handle, now_running);
	    wait_frametime(c_handle, SAFE_FRAME_PERIOD);
	    camwire_flush_framebuffers(c_handle, num_buffers, NULL,
				       NULL);
	    if (set->acqtype == single)
	    {
		now_single = 1;
		camwire_set_single_shot(c_handle, now_single);
	    }
	}
    }
}
 
/*
  ----------------------------------------------------------------------
  The name says it all really.
*/
static void save_numbered_image(const Camwire_handle c_handle,
				const void *fb,
				const int image_type,
				Settings_t *set,
				FILE *logfile)
{
    long framenumber;
    char numbered_name[MAX_FILENAME+1];
    char *nameptr;
    int depth;
    FILE *imagefile;
    /* int image_fd;*/
    char errormessage[MAX_MESSAGE+1];
    char fileheader[MAX_IMAGE_HEADER+1];
    struct timespec timestamp;
    char timestamp_str[MAX_TIMESTAMP+1];
    ssize_t byteswritten;
    int buffer_lag;
    unsigned int max_pixel;

    /* Generate a filename: */
    camwire_get_framenumber(c_handle, &framenumber);
    filename_framenumber(numbered_name, set->imagefilename, 0,
			 framenumber);
    nameptr = &numbered_name[strlen(numbered_name)-4];
    if (strncmp(nameptr, ".ppm", 4) != 0 &&
	strncmp(nameptr, ".pgm", 4) != 0)
    {
	if (image_type == 5)       strcat(numbered_name, ".pgm");
	else if (image_type == 6)  strcat(numbered_name, ".ppm");
    }
    
    /* Decide on max pixel size to save: */
    camwire_pixel_depth(set->coding, &depth);
    switch (depth)
    {
	case 8:
	case 24:
	    max_pixel = 255;
	    break;
	case 16:
	case 48:
	    max_pixel = 65535;
	    break;
	case 12:
	default:
	    max_pixel = 0; 	/* YUV411?*/
	    break;
    }

    /* Write the frame to an image file:*/
    imagefile = fopen(numbered_name, "w");
    if (imagefile == NULL)
    /*
    image_fd = open(numbered_name, O_WRONLY | O_CREAT | O_TRUNC);
    if (image_fd < 0)
    */
    {
	snprintf(errormessage, MAX_MESSAGE, "Can't create '%s'",
		 numbered_name);
	perror(errormessage);
	return;
    }
    camwire_get_framenumber(c_handle, &framenumber);
    camwire_get_timestamp(c_handle, &timestamp);
    snprintf(timestamp_str, MAX_TIMESTAMP, "%.6lf",
	     timestamp.tv_sec + 1.0e-9*timestamp.tv_nsec);
    snprintf(fileheader, MAX_IMAGE_HEADER,
	     "P%d\n%c frame %ld, \ttime %s\n%d %d\n%d\n",
	     image_type, SETTINGS_COMMENT, framenumber, timestamp_str,
	     set->width, set->height, max_pixel);
    
    byteswritten = fprintf(imagefile, "%s", fileheader);
    if (byteswritten > 0)
    {
	byteswritten = fwrite((const char *) fb,
			      (size_t) (depth/8),
			      (size_t) set->width*set->height,
			      imagefile);
    }
    if (use_fsync)  fsync(fileno(imagefile));
    fclose(imagefile);

    /*
    byteswritten = write(image_fd, fileheader, strlen(fileheader));
    if (byteswritten > 0)
    {
	byteswritten =
	    write(image_fd, (const char *) fb,
		  (size_t) set->width*set->height*depth/8);
    }
    if (use_fsync)  fsync(iamge_fd);
    close(image_fd);
    */

    if (byteswritten > 0)
    {
	camwire_get_framebuffer_lag(c_handle, &buffer_lag);
	fprintf(logfile, "%11ld \t%s \t%4d\n", framenumber,
		timestamp_str, buffer_lag);
	fflush(logfile);
    }
    else
    {
	snprintf(errormessage, MAX_MESSAGE, "Could not write to '%s'",
		 numbered_name);
	perror(errormessage);
    }
}
 
/*
  ----------------------------------------------------------------------
  If strip is 0, expands the given filename by inserting an
  11-decimal-digit frame number at the first `%d' (or not if it does not
  include a `%d').  If strip is not 0, strips the given filename of any
  `%d' token.  Returns the expanded or stripped filename in new_name.
*/
static void filename_framenumber(char *new_name,
				 const char *filename,
				 const int strip,
				 const long frameno)
{
    size_t namelen, tokenindex;
    char *token;

    namelen = strlen(filename);
    token = strstr(filename, FRAMENUMBER_TOKEN);
    if (token == NULL)  tokenindex = namelen;
    else                tokenindex = token - filename;
    strncpy(new_name, filename, tokenindex);
    new_name[tokenindex] = '\0';
    if (token != NULL)
    {
	if (!strip)  sprintf(&new_name[tokenindex], "%011ld", frameno);
	tokenindex += strlen(FRAMENUMBER_TOKEN);
    }
    strcat(new_name, &filename[tokenindex]);
}

/*
  ----------------------------------------------------------------------
  Checks the number of pending filled buffers and flushes some of them
  if there are too many.  This helps to ensure that frame numbers are
  accurately updated when frames are dropped.  Otherwise buffer
  overflows may result in the user not knowing if frames have been lost.
*/
static void manage_buffer_level(const Camwire_handle c_handle,
				FILE *logfile)
{
    int total_frames, current_level, num_to_flush;

    camwire_get_num_framebuffers(c_handle, &total_frames);
    if (total_frames < 3)  return;
    camwire_get_framebuffer_lag(c_handle, &current_level);
    current_level++; 	/* Buffer lag does not count current frame.*/

    /* It seems that the DMA buffers sometimes do not fill up
       completely, hence the extra -1 in the if expression below: */
    if (current_level >= total_frames - 1)
    { 	/* Hit the ceiling.*/
	num_to_flush = total_frames;
	camwire_flush_framebuffers(c_handle, num_to_flush, NULL, NULL);
	if (logfile != NULL)
	{
	    fprintf(logfile, "Frame buffers overflowed.  "
		    "Frame numbers may no longer be in synch.\n");
	    fflush(logfile);
	}
    }
    else if (current_level + 0.5 >= BUF_HIGH_MARK*total_frames)
    {
	num_to_flush = current_level - BUF_LOW_MARK*total_frames;
	camwire_flush_framebuffers(c_handle, num_to_flush, NULL, NULL);
    }
    /* else don't flush. */
}

/*
  ----------------------------------------------------------------------
  Returns a pointer to a pixel colour coding string corresponding to
  the Camwire pixel coding.
*/
static char * pixelcoding2string(const Camwire_pixel coding)
{
    char *str;

    switch (coding)
    {	
	case CAMWIRE_PIXEL_MONO8:
	    str = "MONO8";
	    break;
	case CAMWIRE_PIXEL_YUV411:
	    str = "YUV411";
	    break;
	case CAMWIRE_PIXEL_YUV422:
	    str = "YUV422";
	    break;
	case CAMWIRE_PIXEL_YUV444:
	    str = "YUV444";
	    break;
	case CAMWIRE_PIXEL_RGB8:
	    str = "RGB8";
	    break;
	case CAMWIRE_PIXEL_MONO16:
	    str = "MONO16";
	    break;
	case CAMWIRE_PIXEL_RGB16:
	    str = "RGB16";
	    break;
	case CAMWIRE_PIXEL_MONO16S:
	    str = "MONO16S";
	    break;
	case CAMWIRE_PIXEL_RGB16S:
	    str = "RGB16S";
	    break;
	case CAMWIRE_PIXEL_RAW8:
	    str = "RAW8";
	    break;
	case CAMWIRE_PIXEL_RAW16:
	    str = "RAW16";
	    break;
	case CAMWIRE_PIXEL_INVALID:
	default:
	    str = "INVALID";
	    break;
    }
    return(str);
}

/*
  ----------------------------------------------------------------------
  Returns the Camwire pixel coding corresponding to a pixel colour
  coding string.  Case-insensitive.
*/
static Camwire_pixel string2pixelcoding(const char *str)
{
    if      (!stricmp(str, "MONO8"))  	return(CAMWIRE_PIXEL_MONO8);
    else if (!stricmp(str, "YUV411"))  	return(CAMWIRE_PIXEL_YUV411);
    else if (!stricmp(str, "YUV422"))  	return(CAMWIRE_PIXEL_YUV422);
    else if (!stricmp(str, "YUV444"))  	return(CAMWIRE_PIXEL_YUV444);
    else if (!stricmp(str, "RGB8"))  	return(CAMWIRE_PIXEL_RGB8);
    else if (!stricmp(str, "MONO16"))  	return(CAMWIRE_PIXEL_MONO16);
    else if (!stricmp(str, "RGB16"))  	return(CAMWIRE_PIXEL_RGB16);
    else if (!stricmp(str, "MONO16S")) 	return(CAMWIRE_PIXEL_MONO16S);
    else if (!stricmp(str, "RGB16S"))  	return(CAMWIRE_PIXEL_RGB16S);
    else if (!stricmp(str, "RAW8"))  	return(CAMWIRE_PIXEL_RAW8);
    else if (!stricmp(str, "RAW16"))  	return(CAMWIRE_PIXEL_RAW16);
    else  				return(CAMWIRE_PIXEL_INVALID);
}

/*
  ----------------------------------------------------------------------
  Saves or loads the current monitor settings to or from file.
*/
static void settings_save_load(const Camwire_handle c_handle,
			       Settings_t *set)
{
    int save, gotinput;
    char userinput[MAX_KEY_INPUT+1];
    char key;
    char formatstring[20];
    char settings_filename[MAX_FILENAME+1];

    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT); 	/* Limit the
							 * input field
							 * width.*/
    save = 0;
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Save/Load settings: ");
	fflush(stdout);
	scanf(formatstring, userinput); 	/* Safe input.*/
	key = toupper(userinput[0]);
	if (key == 'S')
	{
	    save = 1;
	    gotinput = 1;
	}
	else if (key == 'L')
	{
	    save = 0;
	    gotinput = 1;
	}
	else
	{
	    printf("Type either S or L.\n");
	    fflush(stdout);
	    clear_stdin();
	}
    }
    
    sprintf(formatstring, "%%%ds", MAX_FILENAME); 	/* Limit the
							 * input field
							 * width.*/
    printf("\nSettings filename: ");
    fflush(stdout);
    scanf(formatstring, settings_filename);

    if (save)  settings_save(set, settings_filename);
    else       settings_load(c_handle, set, settings_filename);
}

/*
  ----------------------------------------------------------------------
  Saves the current monitor settings to file.
*/
static void settings_save(Settings_t *set, const char *settings_filename)
{
    FILE *settingsfile;

    settingsfile = fopen(settings_filename, "w");
    if (settingsfile != NULL)
    {
	fprintf(settingsfile, "%c Cammonitor run-time settings\n\n",
		SETTINGS_COMMENT);
	fprintf(settingsfile, settings_format,
		set->cam_access,
		set->activity,
		set->acqtype,
		set->trigger,
		set->polarity,
		set->shutter,
		set->framerate,
		set->width,
		set->height,
		(int) set->coding,
		set->left,
		set->top,
		set->white_bal_blue,
		set->white_bal_red,
		set->num_buffers,
		set->framenumber,
		set->save_num_images,
		set->save_delay,
		set->imagefilename);
	
	fclose(settingsfile);
	printf("Settings saved to \"%s\".\n", settings_filename);
	fflush(stdout);
    }
    else
    {
	fprintf(stderr, "Could not open the settings file \"%s\" "
		"for writing.\n", settings_filename);
    }
}

/*
  ----------------------------------------------------------------------
  Loads the current monitor settings from file and updates the
  camera.
*/
static void settings_load(const Camwire_handle c_handle,
			  Settings_t *set,
			  const char *settings_filename)
{
    FILE *settingsfile;
    Settings_t new_settings;
    int gotinput;

    settingsfile = fopen(settings_filename, "r");
    if (settingsfile != NULL)
    {
	/* Read any new settings with old settings as default: */
	new_settings = *set;
	gotinput = update_settings(settingsfile, &new_settings);
	fclose(settingsfile);
	if (gotinput)
	{
	    printf("Settings loaded from \"%s\".\n", settings_filename);
	    
	    /* Write any changes to camera and read back actual: */
	    update_camera(c_handle, &new_settings, set);
	    get_camera_settings(c_handle, set);
	    
	    /* Copy internal monitor settings: */
	    set->save_num_images = new_settings.save_num_images;
	    set->save_delay = new_settings.save_delay;
	    strncpy(set->imagefilename, new_settings.imagefilename,
		    MAX_FILENAME);
	}
	else
	{
	    fprintf(stderr, "No settings read from %s.\n",
		    settings_filename);
	}
    }
    else
    {
	fprintf(stderr, "Could not open the settings file \"%s\" "
		"for reading.\n", settings_filename);
    }
}

/*
  ----------------------------------------------------------------------
  Reads the opened settings file, changing only the fields present in
  the file.  The new settings are assumed already to be initialized with
  old settings.
*/
static int update_settings(FILE *settingsfile, Settings_t *new)
{
    int gotinput, empty, comment;
    char linebuffer[MAX_SETTINGS_LINE+1];
    char *lineptr;
    char *new_tag_str, *value_str;
    char *ref_tag_str, *format_str;

    gotinput = 0;
    while(!feof(settingsfile))
    {
	/* Read lines until not empty and not a comment: */
	empty = 1;
	comment = 1;
	while (empty || comment)
	{
	    lineptr =
		fgets(linebuffer, MAX_SETTINGS_LINE, settingsfile);
	    if (lineptr == NULL)  break;
	    empty = (strlen(skip_whitespace(linebuffer)) == 0);
	    comment = (linebuffer[0] == SETTINGS_COMMENT);
	}
	if (lineptr == NULL)  break;

	/* Separate input tag string and value string: */
	new_tag_str = skip_whitespace(linebuffer);
	lineptr = skip_non_whitespace(new_tag_str);
	*lineptr = '\0';
	lineptr++;
	value_str = skip_whitespace(lineptr);
	if (strlen(value_str) >= 2)  /* Including a newline char.*/
	{
	    /* Find corresponding tag and conversion in the format: */
	    ref_tag_str = strstr(settings_format, new_tag_str);
	    if (ref_tag_str != NULL)
	    {
		lineptr = skip_non_whitespace(ref_tag_str);
		format_str = skip_whitespace(lineptr);
		
		if (strcmp(new_tag_str, "cam_access") == 0)
		{
		    check(sscanf(value_str, format_str,
				 &new->cam_access),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "activity") == 0)
		{
		    check(sscanf(value_str, format_str, &new->activity),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "acqtype") == 0)
		{
		    check(sscanf(value_str, format_str, &new->acqtype),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "trigger") == 0)
		{
		    check(sscanf(value_str, format_str, &new->trigger),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "polarity") == 0)
		{
		    check(sscanf(value_str, format_str, &new->polarity),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "shutter") == 0)
		{
		    check(sscanf(value_str, format_str, &new->shutter),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "framerate") == 0)
		{
		    check(sscanf(value_str, format_str,&new->framerate),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "width") == 0)
		{
		    check(sscanf(value_str, format_str, &new->width),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "height") == 0)
		{
		    check(sscanf(value_str, format_str, &new->height),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "coding") == 0)
		{
		    check(sscanf(value_str, format_str, (int *)
				 &new->coding), new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "left") == 0)
		{
		    check(sscanf(value_str, format_str, &new->left),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "top") == 0)
		{
		    check(sscanf(value_str, format_str, &new->top),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "white_bal_blue") == 0)
		{
		    check(sscanf(value_str, format_str,
				 &new->white_bal_blue),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "white_bal_red") == 0)
		{
		    check(sscanf(value_str, format_str,
				 &new->white_bal_red),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "num_buffers") == 0)
		{
		    check(sscanf(value_str, format_str,
				 &new->num_buffers),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "framenumber") == 0)
		{
		    check(sscanf(value_str, format_str,
				 &new->framenumber),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "save_num_images") == 0)
		{
		    check(sscanf(value_str, format_str,
				 &new->save_num_images),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "save_delay") == 0)
		{
		    check(sscanf(value_str,format_str,&new->save_delay),
			  new_tag_str, &gotinput);
		}
		else if (strcmp(new_tag_str, "imagefilename") == 0)
		{
		    check(sscanf(value_str, format_str,
				 new->imagefilename),
			  new_tag_str, &gotinput);
		}
                else
		{
		    fprintf(stderr, "Cammonitor internal error:  Don't "
			    "know how to read tag `%s' in the settings "
			    "format string.\n", new_tag_str);
		}
	    }
	    else
	    {
		fprintf(stderr,
			"Unrecognized tag `%s' in settings file.\n",
			new_tag_str);
	    }
	}
	else
	{
	    fprintf(stderr, "No value for tag `%s' in settings file.\n",
		    new_tag_str);
	}
    }  /* while */

    return(gotinput);
}

/*
  ----------------------------------------------------------------------
  Sets gotinput if scanresult is 1, else prints error message.
*/
static void check(const int scanresult, const char *tag, int *gotinput)
{
    if (scanresult == 1)
    {
	*gotinput = 1;
    }
    else
    {
	fprintf(stderr,
		"Error reading value of `%s' in settings file.\n", tag);
    }
}

/*
  ----------------------------------------------------------------------
  Return a pointer to the 1st non-whitespace character in null-delimited
  string.  If the string contains only whitespace, returns a pointer to
  an empty string (pointer to the null character at the end of the
  string).
*/
static char * skip_whitespace(const char *string)
{
    return((char *) &string[strspn(string, WHITESPACE)]);
}

/*
  ----------------------------------------------------------------------
  Return a pointer to the 1st whitespace character in null-delimited
  string.  If the string contains no whitespace, returns a pointer to an
  empty string (pointer to the null character at the end of the string).
*/
static char * skip_non_whitespace(const char *string)
{
    return((char *) &string[strcspn(string, WHITESPACE)]);
}

/*
  ----------------------------------------------------------------------
  Update the camera with new settings.  Note that the order of register
  writes may be significant for some cameras after power-up or
  reset/initilize, hence the elaborate cascade.
*/
static void update_camera(const Camwire_handle c_handle,
			  Settings_t *new,
			  const Settings_t *set)
{
    int dotherest;
	
    dotherest = 0;
    if (dotherest ||
	new->num_buffers != set->num_buffers)
    {
	camwire_set_num_framebuffers(c_handle, new->num_buffers);
	dotherest = 1;
    }
    if (dotherest ||
	new->left != set->left || new->top != set->top)
    {
	camwire_set_frame_offset(c_handle, new->left, new->top);
	dotherest = 1;
    }
    if (dotherest ||
	new->width != set->width || new->height != set->height)
    {
	camwire_set_frame_size(c_handle, new->width, new->height);
	dotherest = 1;
    }
    if (dotherest ||
	new->coding != set->coding)
    {
	camwire_set_pixel_coding(c_handle, new->coding);
	dotherest = 1;
    }
    if (dotherest ||
	fabs(new->framerate - set->framerate) > 1e-6)
    {
	camwire_set_framerate(c_handle, new->framerate);
	dotherest = 1;
    }
    if (dotherest ||
	new->trigger != set->trigger)
    {
	camwire_set_trigger_source(c_handle,
				   new->trigger);
	dotherest = 1;
    }
    if (dotherest ||
	new->polarity != set->polarity)
    {
	camwire_set_trigger_polarity(c_handle,
				     new->polarity);
	dotherest = 1;
    }
    if (dotherest ||
	new->shutter != set->shutter)
    {
	camwire_set_shutter(c_handle, new->shutter);
	dotherest = 1;
    }
    if (dotherest ||
	new->white_bal_blue != set->white_bal_blue ||
	new->white_bal_red != set->white_bal_red)
    {
	camwire_set_white_balance(c_handle, new->white_bal_blue,
				  new->white_bal_red);
	dotherest = 1;
    }
    if (dotherest ||
	new->acqtype != set->acqtype)
    {
	camwire_set_single_shot(c_handle, new->acqtype);
	dotherest = 1;
    }
    if (dotherest ||
	new->activity != set->activity)
    {
	camwire_set_run_stop(c_handle, new->activity);
	dotherest = 1;
    }
    if (dotherest ||
	new->cam_access != set->cam_access)
    {
	camwire_set_stateshadow(c_handle, new->cam_access);
	dotherest = 1;
    }
}

/*
  ----------------------------------------------------------------------
  Displays Camwire hardware configuration or current settings.
*/
static void show_camwire_data(const Camwire_handle c_handle)
{
    int getconfig, gotinput;
    char userinput[MAX_KEY_INPUT+1];
    char key;
    char formatstring[20];
    Camwire_conf config;
    Camwire_state settings;

    sprintf(formatstring, "%%%ds", MAX_KEY_INPUT); 	/* Limit the
							 * input field
							 * width.*/
    getconfig = 0;
    gotinput = 0;
    putchar('\n');
    while (!gotinput)
    {
	printf("Display Config/Settings: ");
	fflush(stdout);
	scanf(formatstring, userinput); 	/* Safe input.*/
	key = toupper(userinput[0]);
	if (key == 'C')
	{
	    getconfig = 1;
	    gotinput = 1;
	}
	else if (key == 'S')
	{
	    getconfig = 0;
	    gotinput = 1;
	}
	else
	{
	    printf("Type either C or S.\n");
	    fflush(stdout);
	    clear_stdin();
	}
    }

    if (getconfig)
    {
	camwire_get_config(c_handle, &config);
	camwire_print_config(&config);
    }
    else
    {
	camwire_get_state(c_handle, &settings);
	camwire_write_state_to_file("/dev/stdout", &settings);
    }
}

/*
  ----------------------------------------------------------------------
  Sleeps for multiple times one frame period, where multiple is a float.
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
  Reads and discards a line from stdin.
*/
static void clear_stdin(void)
{
    char throwaway[MAX_KEY_INPUT+1];
    
    fgets(throwaway, MAX_KEY_INPUT, stdin);
}

/*
  ----------------------------------------------------------------------
  Prints a usage message to stderr.
*/
static void usage_message(char * const argv[])
{
    fprintf(stderr,
	    "\n"
	    "Usage:  %s [-s] [-d display_type] [-e settings_filename] "
			"[-c chip_id_string]\n"
	    "where   -s disables image file sync to disk\n"
	    "        -d selects type of screen display "
	    "(NONE | SDL | XV), default is SDL\n"
	    "        -e autoloads the given settings file\n"
	    "        -c specifies a camera by its unique chip ID\n",
	    argv[0]);
}

/*
  ----------------------------------------------------------------------
  Case-insensitive string comparison.
*/

static int stricmp(const char *str1, const char *str2)
{
    int i;

    i = 0;
    while (str1[i] != '\0' && str2[i] != '\0')
    {
	if (toupper(str1[i]) != toupper(str2[i]))  break;
	i++;
    }
    return((int) toupper(str1[i]) - (int) toupper(str2[i]));
}

/*
  ----------------------------------------------------------------------
  Prints the given error message (including the camera number if it is
  non-negative) to stderr, cleans up, and exits with a return status of
  1.
*/
static void errorexit(const Camwire_handle c_handle, const int cam,
		      const char *msg)
{
    fflush(stdout);  /* libdc1394 writes error messages to stdout.*/
    if (cam < 0)  fprintf(stderr, "\n%s\n", msg);
    else          fprintf(stderr, "\nCamera %d: %s\n", cam + 1, msg);
    cleanup(c_handle);
    exit(EXIT_FAILURE);
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
	wait_frametime(c_handle, SAFE_FRAME_PERIOD);
    }
    if (display_initialized)  display_destroy(d_handle);
    display_initialized = 0;
    camwire_destroy(c_handle);
    camwire_bus_destroy();
    putchar('\n');
    fflush(stdout);
}
