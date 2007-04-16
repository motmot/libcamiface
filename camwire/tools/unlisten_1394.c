/*
  Tells the video1394 driver to stop listening to the given channel.
  Compile this file with something like:

  gcc -g -O2 -Wall -W -ldc1394_control -lraw1394 unlisten.c -o unlisten
  
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ieee1394/video1394.h"


#define VIDEO1394_DEVICE_NAME	"/dev/video1394/"
#define DEVICE_NAME_SIZE	50


/****** main **********************************************************/

int main(const int argc, char *argv[])
{
    int port, channel, device_given, fd, retval;
    char *device;
    
    if (argc < 3)
    {
	fprintf(stderr,
"Usage:    %s port_number channel_number [device_name]\n"
"          port_number (0, 1, 2 ...) refers to the host card or chip.\n"
"          channel_number (0, 1, 2 ...) refers to the camera.\n"
"          device_name (default /dev/video1394/port_number)\n"
"                         refers to the video device.\n"
"          If a device_name is given, port_number is ignored.\n"
"Examples: %s 1 0 unlistens the first camera on the second port.\n"
"          %s 0 2 /dev/video1394/0 unlistens the third camera on\n"
"                         the first port using the named device.\n",
		argv[0], argv[0], argv[0]);
	return(EXIT_FAILURE);
    }

    if (argc > 3)  device_given = 1;
    else           device_given = 0;

    if (device_given)
    {
	device = argv[3];
    }
    else
    {
	port = atoi(argv[1]);
	device = (char *) calloc(1, DEVICE_NAME_SIZE);
	snprintf(device, DEVICE_NAME_SIZE, "%s%d",
		 VIDEO1394_DEVICE_NAME, port);
    }

    fd = open(device, O_RDONLY);
    if (fd < 0)
    {
	fprintf(stderr, "Could not open video1394 device %s\n", device);
	return(EXIT_FAILURE);
    }
    channel = atoi(argv[2]);
    retval = ioctl(fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &channel);
    close(fd);

    if (!device_given)  free(device);
    return(retval);
}
