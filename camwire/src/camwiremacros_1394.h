#ifndef CAMWIREMACROS_1394_H
#define CAMWIREMACROS_1394_H
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


    Title: Macro header for Camwire modules

    Description:
    This header file provides some macros for printing debugging
    information and for error status returns to make the code more
    readable and maintainable.  This header should only be included in
    implementation-specific files, i.e. into files that contain `1394'
    in the file name.

***********************************************************************/


#include <stdio.h>

#include "camwire/camwire.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Print debugging error message: */
#ifdef CAMWIRE_DEBUG
#define DPRINTF(m) \
fprintf(stderr,"%s (%d): %s\n",__FILE__,__LINE__,(m)); fflush(stderr)
#else
#define DPRINTF(m)
#endif

/* Function error status returns, for readability and
   maintainability: */
#define ERROR_IF_NULL(p) \
    if ((p)==NULL) {DPRINTF("Null pointer."); return(CAMWIRE_FAILURE);}
#define ERROR_IF_ZERO(v) \
    if ((v)==0) {DPRINTF("Bad (zero) value."); return(CAMWIRE_FAILURE);}
#define ERROR_IF_CAMWIRE_FAIL(r) \
    if ((r)!=CAMWIRE_SUCCESS) \
      {DPRINTF("Camwire function call failed."); \
       return(CAMWIRE_FAILURE);}
#define ERROR_IF_DC1394_FAIL(r)	\
    if ((r)!=DC1394_SUCCESS) \
      {DPRINTF("dc1394 function call failed."); \
       return(CAMWIRE_FAILURE);}

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
}
#endif

#endif
