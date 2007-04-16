/*
  Resets the IEEE 1394 bus.  Compile this with something like:

gcc -g -O2 -Wall -W -I../include -lraw1394 -ldc1394 resetbus.c ../src/camwirebus_1394.o -o resetbus


***********************************************************************/


#include "camwire/camwirebus.h"


/****** main **********************************************************/

int main(void)
{
    camwire_bus_reset();
    return(0);
}
