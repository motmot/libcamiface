/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: 

    debug.h

Abstract


Author:

    Peter Binder (pbinder) 4/08/98

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/08/98  pbinder   birth
--*/
#ifdef __cplusplus
extern "C" {
#endif

#define	TL_TRACE    0
#define	TL_WARNING  1
#define TL_ERROR    2
#define TL_FATAL    3

#ifdef _DEBUG_C
unsigned char TraceLevel = TL_TRACE;
#else
extern unsigned char TraceLevel;
#endif

#define TRACE(tl, x)                \
    if ( (tl) >= TraceLevel ) {     \
        DbgPrt x ;                  \
    }

void
DbgPrt(
    IN HANDLE   hWnd,
    IN PUCHAR   lpszFormat, 
    IN ... 
    );

void DllTrace(int nlevel,const char *format, ...);

#ifdef __cplusplus
}
#endif
