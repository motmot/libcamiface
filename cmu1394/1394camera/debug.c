/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: 

    debug.c

Abstract


Authors:

    Peter Binder (pbinder) 4/08/98
	Christopher Baker (cbaker) 10/01

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
4/08/98  pbinder   birth
10/xx/01 cbaker    the old tracing mechanism was funky and required that the 
                   dll be built with debug symbols, so I made a new one.
--*/

#define _DEBUG_C
#include "pch.h"
#undef _DEBUG_C

#include <commctrl.h>
#include "resource.h"

extern HINSTANCE g_hInstDLL;

void 
DbgPrt(
    IN HANDLE   hWnd,
    IN PUCHAR   lpszFormat,
    IN ... 
    )
{
    char    buf[1024] = "1394API: ";
    va_list ap;

    va_start(ap, lpszFormat);

    wvsprintf( &buf[9], lpszFormat, ap );
#ifdef _DEBUG
    OutputDebugStringA(buf);
#endif
/*    if (hWnd)
        WriteTextToEditControl(hWnd, buf);
*/
    va_end(ap);
}

/*
 * this is cbaker's tracing mechanism
 * it is alive all the time, so users can turn
 * up the tracelevel and see what the library
 * is complaining about
 */

int DllTraceLevel = DLL_TRACE_VERBOSE;
void CAMAPI SetDllTraceLevel(int nlevel)
{
	DllTraceLevel = nlevel;
}

void DllTrace(int nlevel,const char *format, ...)
{

	if(nlevel > DllTraceLevel)
	{
		return;
	} else {
	    char buf[2048] = "1394Camera.dll: ";
		va_list ap;

		va_start(ap, format);
		wvsprintf(buf + 16, format, ap);
		OutputDebugStringA(buf);

		va_end(ap);
	}
}


/*
 * DebugDlgProc
 *
 * Dialog Procedure for the Camera Debug Settings Dialog
 *
 * Responsibilities:
 *  - catch SCROLL messages and feed them back to the static texts
 *  - save the settings to the registry if necessary
 */

HRESULT CALLBACK DebugDlgProc(
  HWND hWndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	HWND hWndItem;
	DWORD dwPos;
	HKEY myKey = NULL;
	DWORD dwDisp,dwRet,dwSize = sizeof(DWORD);

	switch(uMsg)
	{
	case WM_INITDIALOG:
		// set the sliders to range from 0-101
		hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_DLL);
		SendMessage(hWndItem,TBM_SETRANGE,FALSE,MAKELONG(0,101));
		SendMessage(hWndItem,TBM_SETPOS,TRUE,DllTraceLevel + 1);
		SetDlgItemInt(hWndDlg,IDC_DLL_DESCRIPTION,DllTraceLevel,TRUE);

		hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_SYS);
		SendMessage(hWndItem,TBM_SETRANGE,FALSE,MAKELONG(0,101));
		SendMessage(hWndItem,TBM_SETPOS,TRUE,DllTraceLevel + 1);
		SetDlgItemInt(hWndDlg,IDC_SYS_DESCRIPTION,DllTraceLevel,TRUE);

		return TRUE;
		break;
	case WM_HSCROLL:
	case WM_VSCROLL:
		// use the scroll messages to provide feedback about the tracelevel
		// these may eventually be strings, but leave them as numbers for now
		hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_DLL);
		dwPos = SendMessage(hWndItem,TBM_GETPOS,0,0);
		SetDlgItemInt(hWndDlg,IDC_DLL_DESCRIPTION,dwPos - 1,TRUE);
		hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_SYS);
		dwPos = SendMessage(hWndItem,TBM_GETPOS,0,0);
		SetDlgItemInt(hWndDlg,IDC_SYS_DESCRIPTION,dwPos - 1,TRUE);
		return TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_DLL);
			DllTraceLevel = SendMessage(hWndItem,TBM_GETPOS,0,0) - 1;
			if(BST_CHECKED == IsDlgButtonChecked(hWndDlg,IDC_CHECK_DEFAULT))
			{
				// this is where we write the data into the registry
				dwRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,"Software\\CMU\\1394Camera",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&dwDisp);
				hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_SYS);
				dwPos = SendMessage(hWndItem,TBM_GETPOS,0,0);
				if(dwRet != ERROR_SUCCESS)
				{
					DllTrace(DLL_TRACE_ERROR,"DebugDialogProc: Failed to open the key (%d)\n",dwRet);
					myKey = NULL;
				} else {
					dwRet = RegSetValueEx(myKey,"DllTraceLevel",0,REG_DWORD,(LPBYTE)&DllTraceLevel,dwSize);
					if(dwRet != ERROR_SUCCESS)
						DllTrace(DLL_TRACE_ERROR,"DllMain: error setting DllTraceLevel registry key (%d)\n",dwRet);
					dwRet = RegSetValueEx(myKey,"SysTraceLevel",0,REG_DWORD,(LPBYTE)&dwPos,dwSize);
					if(dwRet != ERROR_SUCCESS)
						DllTrace(DLL_TRACE_ERROR,"DllMain: error setting SysTraceLevel registry key (%d)\n",dwRet);
				}
				RegCloseKey(myKey);
			}
		case IDCANCEL:
			EndDialog(hWndDlg,0);
			return TRUE;
			break;
		}
		break;
	}
	return FALSE;
}


/*
 * CameraDebugDialog
 *
 * EXPORTED
 *
 * Pops up the Debug Settings Dialog in a modal fashion.
 *
 * Arguments
 *  - hWndParent: The Parent Window.  NULL is okay, but the dialog becomes
 *      system-modal instead
 */

void
CAMAPI
CameraDebugDialog(HWND hWndParent)
{
	// we need basic common controls to use the trackbar class
	InitCommonControls();
	DialogBox(g_hInstDLL,MAKEINTRESOURCE(IDD_DEBUG),hWndParent,(DLGPROC)(DebugDlgProc));
}
