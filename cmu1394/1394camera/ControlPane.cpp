///////////////////////////////////////////////////////////////////////////////
//
// ControlPane.cpp
//
// source for CameraControlDialog and associated functions
//
//  Copyright 8/2002
//
//  Christopher Baker
//  Robotics Institute
//  Carnegie Mellon University
//  Pittsburgh, PA
//
//  This file is part of the CMU 1394 Digital Camera Driver
//
//  The CMU 1394 Digital Camera Driver is free software; you can redistribute 
//  it and/or modify it under the terms of the GNU Lesser General Public License 
//  as published by the Free Software Foundation; either version 2.1 of the License,
//  or (at your option) any later version.
//
//  The CMU 1394 Digital Camera Driver is distributed in the hope that it will 
//  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with the CMU 1394 Digital Camera Driver; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//////////////////////////////////////////////////////////////////////


extern "C" {
#include "pch.h"
}
#include "1394Camera.h"
#include "ControlDialog.h"
#include "resource.h"
#include <stdio.h>

/*
 * Local Function Prototypes
 */

LRESULT CALLBACK ControlPaneDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * CreatePane
 *
 * Exposed to other source files by ControlDialog.h
 *
 * Adds a Pane to the window, and does the various bookkeeping
 * neccessary to do so.
 *
 * Arguments:
 * - hInstance: Handle to the instance that contains the IDD_CONTROL_PANE resource
 * - hWndParent: Handle to the window that will host the child pane.
 * - nIndex: Deprecated, will likely become PCONTROL_PANE_EXTENSION
 */

BOOL CreatePane(HINSTANCE hInstance, HWND hWndParent, int nIndex)
{
   HWND hWnd=NULL;

   DllTrace(DLL_TRACE_ENTER,"ENTER CreatePane (%08x,%08x,%08x)\n",
	   hInstance,hWndParent,nIndex);

   hWnd = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_CONTROL_PANE),hWndParent,(DLGPROC)ControlPaneDlgProc,nIndex);

   if (!hWnd)
   {
	   DllTrace(DLL_TRACE_ERROR,"CreateDialogParam Failed (%d)",GetLastError());
	   DllTrace(DLL_TRACE_EXIT,"EXIT CreatePane (FALSE)\n");
	   return FALSE;
   }

   DllTrace(DLL_TRACE_EXIT,"EXIT CreatePane (TRUE)\n");
   return TRUE;
}

/*
 * ControlPaneDlgProc
 *
 * The Window Message procedure for the individual panes in the control dialog
 *
 * Follows the DLGPROC function prototype
 */

LRESULT CALLBACK ControlPaneDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PCONTROL_PANE_EXTENSION pPaneExt = (PCONTROL_PANE_EXTENSION)(GetWindowLong(hWnd,GWL_USERDATA));
	HWND hWndItem;
	LONG lRes;
	LONG min,max,curr1,curr2;
	LRESULT lRetval = FALSE;

	DllTrace(DLL_TRACE_ENTER,"ENTER ControolPaneDlgProc(%08x,%08x,%08x,%08x\n",
		hWnd, message, wParam, lParam);

	switch(message)
	{
	case WM_INITDIALOG:
		// pPaneExt isn't in GWL_USERDATA, it's lParam
		pPaneExt = (PCONTROL_PANE_EXTENSION) LocalAlloc(LPTR,sizeof(CONTROL_PANE_EXTENSION));
		CopyMemory(pPaneExt,(unsigned char *)(lParam),sizeof(CONTROL_PANE_EXTENSION));
		SetWindowLong(hWnd,GWL_USERDATA,(LPARAM)(pPaneExt));

		// min,max,initval should be gotten from the camera registers

		if(pPaneExt->pControl)
		{
			pPaneExt->pControl->Inquire();
			pPaneExt->pControl->Status();
			min = pPaneExt->slider_min = pPaneExt->pControl->m_min;
			max = pPaneExt->slider_max = pPaneExt->pControl->m_max;
			curr1 = max - (pPaneExt->pControl->m_value1 - min);
			curr2 = max - (pPaneExt->pControl->m_value2 - min);
		} else {
			min = pPaneExt->slider_min = 0;
			max = pPaneExt->slider_max = 100;
			curr1 = curr2 = 50;
		}

		if(pPaneExt->flags & PIF_TWO_SLIDERS)
		{
			RECT wRect,sRect;
			GetWindowRect(hWnd,&wRect);

			// do the slider
			hWndItem = GetDlgItem(hWnd,IDC_SLIDER2);
			SendMessage(hWndItem,TBM_SETRANGE,(WPARAM)FALSE,(LPARAM)MAKELONG(min,max));
			SendMessage(hWndItem,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)curr2);
			ShowWindow(hWndItem,SW_SHOW);
			GetWindowRect(hWndItem,&sRect);
			hWndItem = GetDlgItem(hWnd,IDC_SLIDER1);
		
			MoveWindow(hWndItem,wRect.right - sRect.right,sRect.top - wRect.top,
				sRect.right - sRect.left, sRect.bottom - sRect.top,FALSE);

			// do the feedback window
			hWndItem = GetDlgItem(hWnd,IDC_SLIDER_FEEDBACK2);
			ShowWindow(hWndItem,SW_SHOW);
			GetWindowRect(hWndItem,&sRect);
			hWndItem = GetDlgItem(hWnd,IDC_SLIDER_FEEDBACK1);
		
			MoveWindow(hWndItem,wRect.right - sRect.right,sRect.top - wRect.top,
				sRect.right - sRect.left, sRect.bottom - sRect.top,FALSE);

			SetDlgItemInt(hWnd,IDC_SLIDER_FEEDBACK2,pPaneExt->pControl->m_value2,TRUE);

		}



		// init the slider according to the Paneinfo structure
		hWndItem = GetDlgItem(hWnd,IDC_SLIDER1);
		SendMessage(hWndItem,TBM_SETRANGE,(WPARAM)FALSE,(LPARAM)MAKELONG(min,max));
		SendMessage(hWndItem,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)curr1);

		// set the title and initial feedback statics
		SetDlgItemText(hWnd,IDC_TITLE,pPaneExt->pane_name);
		SetDlgItemInt(hWnd,IDC_SLIDER_FEEDBACK1,pPaneExt->pControl->m_value1,TRUE);

		// update our windowID to be the one assigned
		// this is necessary because this child is a dialog per-se
		// and we don't have an opportunity to use the HMENU part
		// of CreateWindow To assign one off-the-bat
		SetWindowLong(hWnd,GWL_ID,pPaneExt->window_id);

		// now set all the bitflags
		// they are all initially disabled

		if(pPaneExt->pControl)
		{
			// the inquiry register
			hWndItem = GetDlgItem(hWnd,IDC_INQ_PRES);
			EnableWindow(hWndItem,pPaneExt->pControl->m_present);
			hWndItem = GetDlgItem(hWnd,IDC_INQ_ONEPUSH);
			EnableWindow(hWndItem,pPaneExt->pControl->m_onePush);
			hWndItem = GetDlgItem(hWnd,IDC_INQ_READ);
			EnableWindow(hWndItem,pPaneExt->pControl->m_readout);
			hWndItem = GetDlgItem(hWnd,IDC_INQ_ONOFF);
			EnableWindow(hWndItem,pPaneExt->pControl->m_onoff);
			hWndItem = GetDlgItem(hWnd,IDC_INQ_AUTO);
			EnableWindow(hWndItem,pPaneExt->pControl->m_auto);
			hWndItem = GetDlgItem(hWnd,IDC_INQ_MANUAL);
			EnableWindow(hWndItem,pPaneExt->pControl->m_manual);

			// the status register
			hWndItem = GetDlgItem(hWnd,IDC_STA_PRES);
			EnableWindow(hWndItem,pPaneExt->pControl->m_present);
			hWndItem = GetDlgItem(hWnd,IDC_STA_ONEPUSH);
			EnableWindow(hWndItem,pPaneExt->pControl->m_statusOnePush);
			hWndItem = GetDlgItem(hWnd,IDC_STA_ONOFF);
			EnableWindow(hWndItem,pPaneExt->pControl->m_statusOnOff);
			hWndItem = GetDlgItem(hWnd,IDC_STA_AUTO);
			EnableWindow(hWndItem,pPaneExt->pControl->m_statusAutoManual);
			hWndItem = GetDlgItem(hWnd,IDC_STA_MANUAL);
			EnableWindow(hWndItem,!pPaneExt->pControl->m_statusAutoManual);
		}


		lRetval = TRUE;
		break;
	case WM_HSCROLL:
	case WM_VSCROLL:

		// the scroll messages come only from the slider, so update
		// the feedback static
		hWndItem = GetDlgItem(hWnd,IDC_SLIDER1);
		lRes = SendMessage(hWndItem,TBM_GETPOS,0,0);
		SetDlgItemInt(hWnd,IDC_SLIDER_FEEDBACK1, pPaneExt->slider_max - (lRes - pPaneExt->slider_min), TRUE);
		if(pPaneExt->pControl)
			pPaneExt->pControl->m_value1 = (unsigned short) (pPaneExt->slider_max - (lRes - pPaneExt->slider_min));


		if(pPaneExt->flags & PIF_TWO_SLIDERS)
		{
			hWndItem = GetDlgItem(hWnd,IDC_SLIDER2);
			lRes = SendMessage(hWndItem,TBM_GETPOS,0,0);
			SetDlgItemInt(hWnd,IDC_SLIDER_FEEDBACK2, pPaneExt->slider_max - (lRes - pPaneExt->slider_min), TRUE);
			if(pPaneExt->pControl)
				pPaneExt->pControl->m_value2 = (unsigned short) (pPaneExt->slider_max - (lRes - pPaneExt->slider_min));
		}

		if(pPaneExt->pControl)
			pPaneExt->pControl->SetValues();

		lRetval = TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BUT_ADVANCED:
			MessageBox(hWnd,"Advanced controls are not yet implemented","1394 Camera Control Pane",MB_OK|MB_ICONERROR);
			lRetval = TRUE;
			break;
		case IDC_BUT_ONOFF:
			if(pPaneExt->pControl)
			{
				pPaneExt->pControl->Status();
				pPaneExt->pControl->TurnOn(!pPaneExt->pControl->m_statusOnOff);
				pPaneExt->pControl->Status();
				hWndItem = GetDlgItem(hWnd,IDC_STA_ONOFF);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusOnOff);
			}

			lRetval = TRUE;
			break;
		case IDC_BUT_AUTO_MAN:
			if(pPaneExt->pControl)
			{
				pPaneExt->pControl->Status();
				pPaneExt->pControl->SetAutoMode(!pPaneExt->pControl->m_statusAutoManual);
				pPaneExt->pControl->Status();
				hWndItem = GetDlgItem(hWnd,IDC_STA_AUTO);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusAutoManual);
				hWndItem = GetDlgItem(hWnd,IDC_STA_MANUAL);
				EnableWindow(hWndItem,!pPaneExt->pControl->m_statusAutoManual);
			}
			lRetval = TRUE;
			break;
		case IDC_BUT_ONEPUSH:
			if(pPaneExt->pControl)
			{
				pPaneExt->pControl->SetOnePush();
				pPaneExt->pControl->Status();
				hWndItem = GetDlgItem(hWnd,IDC_STA_ONEPUSH);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusOnePush);
			}
			lRetval = TRUE;
			break;
		case IDC_BUT_POLL:
			if(pPaneExt->pControl)
			{
				pPaneExt->pControl->Inquire();
				pPaneExt->pControl->Status();

				// the inquiry register
				hWndItem = GetDlgItem(hWnd,IDC_INQ_PRES);
				EnableWindow(hWndItem,pPaneExt->pControl->m_present);
				hWndItem = GetDlgItem(hWnd,IDC_INQ_ONEPUSH);
				EnableWindow(hWndItem,pPaneExt->pControl->m_onePush);
				hWndItem = GetDlgItem(hWnd,IDC_INQ_READ);
				EnableWindow(hWndItem,pPaneExt->pControl->m_readout);
				hWndItem = GetDlgItem(hWnd,IDC_INQ_ONOFF);
				EnableWindow(hWndItem,pPaneExt->pControl->m_onoff);
				hWndItem = GetDlgItem(hWnd,IDC_INQ_AUTO);
				EnableWindow(hWndItem,pPaneExt->pControl->m_auto);
				hWndItem = GetDlgItem(hWnd,IDC_INQ_MANUAL);
				EnableWindow(hWndItem,pPaneExt->pControl->m_manual);

				// the status register
				hWndItem = GetDlgItem(hWnd,IDC_STA_PRES);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusPresent);
				hWndItem = GetDlgItem(hWnd,IDC_STA_ONEPUSH);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusOnePush);
				hWndItem = GetDlgItem(hWnd,IDC_STA_ONOFF);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusOnOff);
				hWndItem = GetDlgItem(hWnd,IDC_STA_AUTO);
				EnableWindow(hWndItem,pPaneExt->pControl->m_statusAutoManual);
				hWndItem = GetDlgItem(hWnd,IDC_STA_MANUAL);
				EnableWindow(hWndItem,!pPaneExt->pControl->m_statusAutoManual);

				curr1 = pPaneExt->slider_max - (pPaneExt->pControl->m_value1 - pPaneExt->slider_min);
				curr2 = pPaneExt->slider_max - (pPaneExt->pControl->m_value2 - pPaneExt->slider_min);
				hWndItem = GetDlgItem(hWnd,IDC_SLIDER1);
				SendMessage(hWndItem,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)curr1);
				SetDlgItemInt(hWnd,IDC_SLIDER_FEEDBACK1, pPaneExt->pControl->m_value1, TRUE);
				if(pPaneExt->flags & PIF_TWO_SLIDERS)
				{
					hWndItem = GetDlgItem(hWnd,IDC_SLIDER2);
					SendMessage(hWndItem,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)curr2);
					SetDlgItemInt(hWnd,IDC_SLIDER_FEEDBACK2, pPaneExt->pControl->m_value2, TRUE);
				}
			}

			lRetval = TRUE;
			break;
		} // switch(command)
		break;
	case WM_DESTROY:
		DllTrace(DLL_TRACE_CHECK,"ControlPaneDlgProc: WM_DESTROY: Freeing %08x\n",pPaneExt);
		LocalFree(pPaneExt);
		break;
	}

	DllTrace(DLL_TRACE_EXIT,"EXIT ControlPaneDlgProc (%s)\n",lRetval ? "TRUE" : "FALSE");
	return lRetval;
}

/*
 * to add:
 *  - Advanced dialog DlgProc and controls
 */