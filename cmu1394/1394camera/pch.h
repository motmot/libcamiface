/*++

Copyright (c) 1998	Microsoft Corporation

Module Name: 

    pch.h

Abstract


Author:

    Peter Binder (pbinder) 4/08/98

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/08/98  pbinder   birth
--*/



#include <windows.h>
#include <setupapi.h>
#include <stdlib.h>
#include "1394camapi.h"
#include "debug.h"

// g_hInstDLL gets set to the DLL Instance on every DLL_PROCESS_ATTACH
// it must be used when referencing resources contained in the DLL
// the actual instance is defined in 1394main.c
extern HINSTANCE g_hInstDLL;

#ifndef ULONG_PTR
#define ULONG_PTR unsigned long *
#endif
