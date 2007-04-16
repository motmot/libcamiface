///////////////////////////////////////////////////////////////////////////////
//
// ControlDialog.cpp
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

static char *g_ColorCodeNames[8] = 
{
	"0 - 8-bit Mono",
	"1 - YUV 4:1:1",
	"2 - YUV 4:2:2",
	"3 - YUV 4:4:4",
	"4 - 24-bit RGB",
	"5 - 16-bit Mono",
	"6 - 48-bit RGB",
	"7 - Unknown"
};

LRESULT CALLBACK ControlSizeDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static void ApplyValues(HWND hWndDlg)
{
	unsigned long top,left,width,height,color,bytes;
	C1394Camera *pCamera = (C1394Camera *)GetWindowLong(hWndDlg,GWL_USERDATA);
	char buf[256];

	width = SendDlgItemMessage(hWndDlg,IDC_SLIDER_WIDTH,TBM_GETPOS,0,0);
	height = SendDlgItemMessage(hWndDlg,IDC_SLIDER_HEIGHT,TBM_GETPOS,0,0);
	left = SendDlgItemMessage(hWndDlg,IDC_SLIDER_LEFT,TBM_GETPOS,0,0);
	top = SendDlgItemMessage(hWndDlg,IDC_SLIDER_TOP,TBM_GETPOS,0,0);
	bytes = SendDlgItemMessage(hWndDlg,IDC_SLIDER_BYTESPACKET,TBM_GETPOS,0,0);

	width *= pCamera->m_controlSize.m_unitH;
	height *= pCamera->m_controlSize.m_unitV;
	left *= pCamera->m_controlSize.m_unitHpos;
	top *= pCamera->m_controlSize.m_unitVpos;
	bytes *= pCamera->m_controlSize.m_bytesPacketMin;

	GetWindowText(GetDlgItem(hWndDlg,IDC_COMBO_COLORCODE),buf,256);
	color = buf[0] - '0';

	pCamera->m_controlSize.SetSize(width,height);
	pCamera->m_controlSize.SetPosition(left,top);
	pCamera->m_controlSize.SetColorCode(color);
	pCamera->m_controlSize.SetBytesPerPacket(bytes);
}

static void Refresh(HWND hWndDlg)
{
	C1394Camera *pCamera = (C1394Camera *)(GetWindowLong(hWndDlg,GWL_USERDATA));
	HWND hWndItem;
	LONG lData;
	int i,ret;
	char buf[64];
	char *ptr = buf;
	// read everything in and write it all out to the controls

	// Get the control status

#if 1
	pCamera->m_controlSize.Inquire();
	pCamera->m_controlSize.Status();
#else
	pCamera->m_controlSize.m_height = 960;
	pCamera->m_controlSize.m_width = 1280;
	pCamera->m_controlSize.m_maxH = 1280;
	pCamera->m_controlSize.m_maxV = 960;
	pCamera->m_controlSize.m_unitH = 320;
	pCamera->m_controlSize.m_unitV = 240;
	pCamera->m_controlSize.m_top = 0;
	pCamera->m_controlSize.m_left = 0;
	pCamera->m_controlSize.m_unitHpos = 20;
	pCamera->m_controlSize.m_unitVpos = 20;
	pCamera->m_controlSize.m_bytesPacketMin = 60;
	pCamera->m_controlSize.m_bytesPacketMax = 240;
	pCamera->m_controlSize.m_bytesPacket = 120;
	pCamera->m_controlSize.m_bytesFrameLow = 1280*960*2;
	pCamera->m_controlSize.m_packetsFrame = (1280*960*2) / (120);
	for(i=0; i<8; i++)
		pCamera->m_controlSize.m_colorCodes[i] = true;
	pCamera->m_controlSize.m_colorCode = 0;
#endif

	// Populate the comboboxes
	
	// Modes
	hWndItem = GetDlgItem(hWndDlg,IDC_COMBO_MODE);
	i = SendMessage(hWndItem,CB_GETCOUNT,NULL,NULL);
	while(i-- > 0)
		SendMessage(hWndItem,CB_DELETESTRING,0,0);

	lData = -1;

	for(i=0; i<8; i++)
		if(pCamera->m_bxAvailableModes[7][i])
		{
			pCamera->ReadQuadlet(0x2e0 + 4*i,(unsigned long *)&lData);
			sprintf(buf,"%d - 0xFFFF%08X",i,lData);
			ret = SendMessage(hWndItem,CB_ADDSTRING,0,(LPARAM)buf);
			if(i == pCamera->GetVideoMode())
				lData = ret;
		}

	SendMessage(hWndItem,CB_SETCURSEL,lData,0);

	// color codes
	hWndItem = GetDlgItem(hWndDlg,IDC_COMBO_COLORCODE);
	i = SendMessage(hWndItem,CB_GETCOUNT,NULL,NULL);
	while(i-- > 0)
		SendMessage(hWndItem,CB_DELETESTRING,0,0);

	lData = 0;

	for(i=0; i<8; i+=2)
		if(pCamera->m_controlSize.m_colorCodes[i])
		{
			ret = SendMessage(hWndItem,CB_ADDSTRING,0,(LPARAM)(g_ColorCodeNames[i]));
			if(i == (int)(pCamera->m_controlSize.m_colorCode))
				lData = ret;
		}

	SendMessage(hWndItem,CB_SETCURSEL,lData,0);

	// WIDTH
	hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_WIDTH);
	lData = MAKELONG(1,pCamera->m_controlSize.m_maxH/pCamera->m_controlSize.m_unitH);
	SendMessage(hWndItem,TBM_SETRANGE,FALSE,lData);
	SendMessage(hWndItem,TBM_SETLINESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPAGESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPOS,TRUE,pCamera->m_controlSize.m_width/pCamera->m_controlSize.m_unitH);
	SetDlgItemInt(hWndDlg,IDC_WIDTH_FEEDBACK,pCamera->m_controlSize.m_width,FALSE);

	// HEIGHT
	hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_HEIGHT);
	lData = MAKELONG(1,pCamera->m_controlSize.m_maxV/pCamera->m_controlSize.m_unitV);
	SendMessage(hWndItem,TBM_SETRANGE,FALSE,lData);
	SendMessage(hWndItem,TBM_SETLINESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPAGESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPOS,TRUE,pCamera->m_controlSize.m_height/pCamera->m_controlSize.m_unitV);
	SetDlgItemInt(hWndDlg,IDC_HEIGHT_FEEDBACK,pCamera->m_controlSize.m_height,FALSE);

	// LEFT
	hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_LEFT);
	lData = MAKELONG(0,(pCamera->m_controlSize.m_maxH - pCamera->m_controlSize.m_width)/pCamera->m_controlSize.m_unitHpos);
	SendMessage(hWndItem,TBM_SETRANGE,FALSE,lData);
	SendMessage(hWndItem,TBM_SETLINESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPAGESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPOS,TRUE,pCamera->m_controlSize.m_left/pCamera->m_controlSize.m_unitHpos);
	SetDlgItemInt(hWndDlg,IDC_LEFT_FEEDBACK,pCamera->m_controlSize.m_left,FALSE);

	// TOP
	hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_TOP);
	lData = MAKELONG(0,(pCamera->m_controlSize.m_maxV - pCamera->m_controlSize.m_height)/pCamera->m_controlSize.m_unitVpos);
	SendMessage(hWndItem,TBM_SETRANGE,FALSE,lData);
	SendMessage(hWndItem,TBM_SETLINESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPAGESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPOS,TRUE,pCamera->m_controlSize.m_top/pCamera->m_controlSize.m_unitVpos);
	SetDlgItemInt(hWndDlg,IDC_TOP_FEEDBACK,pCamera->m_controlSize.m_top,FALSE);


	// Fill out the frame info stuff

	SetDlgItemInt(hWndDlg,IDC_PIXFRAME_FEEDBACK,pCamera->m_controlSize.m_pixelsFrame,FALSE);
	SetDlgItemInt(hWndDlg,IDC_BYTESFRAME_FEEDBACK,pCamera->m_controlSize.m_bytesFrameLow,FALSE);
	hWndItem = GetDlgItem(hWndDlg,IDC_SLIDER_BYTESPACKET);
	lData = MAKELONG(1,pCamera->m_controlSize.m_bytesPacketMax/pCamera->m_controlSize.m_bytesPacketMin);
	SendMessage(hWndItem,TBM_SETRANGE,FALSE,lData);
	SendMessage(hWndItem,TBM_SETLINESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPAGESIZE,FALSE,1);
	SendMessage(hWndItem,TBM_SETPOS,TRUE,pCamera->m_controlSize.m_bytesPacket/pCamera->m_controlSize.m_bytesPacketMin);
	SetDlgItemInt(hWndDlg,IDC_BYTESPACKET_FEEDBACK,pCamera->m_controlSize.m_bytesPacket,FALSE);
	SetDlgItemInt(hWndDlg,IDC_PACKETSFRAME_FEEDBACK,pCamera->m_controlSize.m_packetsFrame,FALSE);

	sprintf(buf,"None");
	if(pCamera->m_controlSize.m_bError1)
	{
		sprintf(buf,"IMG");
		ptr += strlen(buf);
	}

	if(pCamera->m_controlSize.m_bError2)
		sprintf(ptr,"%sBPP",ptr == buf ? "" : ",");

	SetDlgItemText(hWndDlg,IDC_ERROR_FEEDBACK,buf);
}

/*
 * ControlSizeDlgProc
 *
 * Dialog Procedure for the Camera Control Size Dialog
 *
 * Responsibilities:
 *  - catch SCROLL messages and feed them back to the static texts
 *  - handle mode and color code changes
 */

HRESULT CALLBACK ControlSizeDlgProc(
  HWND hWndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	C1394Camera *pCamera = (C1394Camera *)(GetWindowLong(hWndDlg,GWL_USERDATA));
	LONG lData;
	int dwFeedbackID;
	int dwFeedbackVal;
	int dwNewMaxID;
	int dwNewMaxVal;
	int dwScrollVal;
	HWND hWndItem;
	char buf[256];

	switch(uMsg)
	{
	case WM_INITDIALOG:
		// pCamera isn't in GWL_USERDATA, it's lParam
		pCamera = (C1394Camera*) lParam;
		SetWindowLong(hWndDlg,GWL_USERDATA,(LPARAM)(pCamera));
		Refresh(hWndDlg);
		return TRUE;
		break;
	case WM_HSCROLL:
	case WM_VSCROLL:
		// use the scroll messages to provide feedback about the tracelevel
		
		lData = (LONG) GetDlgCtrlID((HWND)lParam);
		DllTrace(DLL_TRACE_CHECK,"ControlSizeDialog:WM_SCROLL: CtlID = %d\n",lData);

		dwScrollVal = SendMessage((HWND)lParam,TBM_GETPOS,NULL,NULL);

		// since most of the work for these sliders is fundamentally the same, 
		// we will catch parameters in the switch statement and process them afterward.
		dwFeedbackID = dwNewMaxID = -1;

		switch(lData)
		{
		case IDC_SLIDER_WIDTH:
			dwFeedbackID = IDC_WIDTH_FEEDBACK;
			dwFeedbackVal = dwScrollVal * pCamera->m_controlSize.m_unitH;
			dwNewMaxID = IDC_SLIDER_LEFT;
			dwNewMaxVal = (pCamera->m_controlSize.m_maxH - dwFeedbackVal) / pCamera->m_controlSize.m_unitHpos;
			break;
		case IDC_SLIDER_HEIGHT:
			dwFeedbackID = IDC_HEIGHT_FEEDBACK;
			dwFeedbackVal = dwScrollVal * pCamera->m_controlSize.m_unitV;
			dwNewMaxID = IDC_SLIDER_TOP;
			dwNewMaxVal = (pCamera->m_controlSize.m_maxV - dwFeedbackVal) / pCamera->m_controlSize.m_unitVpos;
			break;
		case IDC_SLIDER_LEFT:
			dwFeedbackID = IDC_LEFT_FEEDBACK;
			dwFeedbackVal = dwScrollVal * pCamera->m_controlSize.m_unitHpos;
			break;
		case IDC_SLIDER_TOP:
			dwFeedbackID = IDC_TOP_FEEDBACK;
			dwFeedbackVal = dwScrollVal * pCamera->m_controlSize.m_unitVpos;
			break;
		case IDC_SLIDER_BYTESPACKET:
			dwFeedbackID = IDC_BYTESPACKET_FEEDBACK;
			dwFeedbackVal = dwScrollVal * pCamera->m_controlSize.m_bytesPacketMin;
			break;
		default:
			DllTrace(DLL_TRACE_WARNING,"SizeDialog: WM_HSCROLL: warning: unknown slider ID %d\n",lData);
			break;
		}

		DllTrace(DLL_TRACE_CHECK,"SizeDialog: feedback = %d:%d, Newmax = %d:%d\n",
			dwFeedbackID,dwFeedbackVal,dwNewMaxID,dwNewMaxVal);

		if(dwFeedbackID >= 0)
			SetDlgItemInt(hWndDlg,dwFeedbackID,dwFeedbackVal,FALSE);

		if(dwNewMaxID >= 0)
		{
			SendDlgItemMessage(hWndDlg,dwNewMaxID,TBM_SETRANGE,TRUE,MAKELONG(0,dwNewMaxVal));
			dwScrollVal = SendDlgItemMessage(hWndDlg,dwNewMaxID,TBM_GETPOS,NULL,NULL);

			if(dwNewMaxID == IDC_SLIDER_LEFT)
				SetDlgItemInt(hWndDlg,IDC_LEFT_FEEDBACK,dwScrollVal * pCamera->m_controlSize.m_unitHpos,FALSE);
			else if(dwNewMaxID == IDC_SLIDER_TOP)
				SetDlgItemInt(hWndDlg,IDC_TOP_FEEDBACK,dwScrollVal * pCamera->m_controlSize.m_unitVpos,FALSE);
		}

		return TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDAPPLY:
			ApplyValues(hWndDlg);
		case IDREFRESH:
			Refresh(hWndDlg);
			return TRUE;
			break;
		case IDOK:
			ApplyValues(hWndDlg);
		case IDCANCEL:
			EndDialog(hWndDlg,LOWORD(wParam));
			return TRUE;
			break;
		case IDC_COMBO_MODE:
			if(HIWORD(wParam) == CBN_SELENDOK)
			{
				hWndItem = (HWND)lParam;
				lData = SendMessage(hWndItem,CB_GETCURSEL,0,0);
				GetWindowText(hWndItem,buf,256);
				DllTrace(DLL_TRACE_CHECK,"SizeDialog: Mode %d Selected on line %d\n",(int)(buf[0] - '0'),lData);
				lData = buf[0] - '0';
				pCamera->SetVideoMode(lData);
				Refresh(hWndDlg);
			}
			return TRUE;
			break;
		case IDC_COMBO_COLORCODE:
			if(HIWORD(wParam) == CBN_SELENDOK)
			{
				hWndItem = (HWND)lParam;
				lData = SendMessage(hWndItem,CB_GETCURSEL,0,0);
				GetWindowText(hWndItem,buf,256);
				DllTrace(DLL_TRACE_CHECK,"SizeDialog: Color Code %d Selected on line %d\n",(int)(buf[0] - '0'),lData);
				lData = buf[0] - '0';
				pCamera->m_controlSize.SetColorCode(lData);
				Refresh(hWndDlg);
			}
			return TRUE;
			break;
		}
		break;
	}
	return FALSE;
}


/*
 * CameraControlSizeDialog
 *
 * EXPORTED
 *
 * Pops up the Debug Settings Dialog in a modal fashion.
 *
 * Arguments
 *  - hWndParent: The Parent Window.  NULL is okay, but the dialog becomes
 *      system-modal instead
 *  - pCamera: the camera to control
 */

long
CAMAPI
CameraControlSizeDialog(HWND hWndParent, 
						C1394Camera *pCamera)
{
	// we need basic common controls to use the trackbar class
	if(!pCamera->m_bxAvailableFormats[7])
	{
		DllTrace(DLL_TRACE_ERROR,"ControlSizeDialog: Camera at %08x does not support format 7!\n");
		return -1;
	}

	InitCommonControls();
	return DialogBoxParam(g_hInstDLL,MAKEINTRESOURCE(IDD_PARTIAL_SCAN),hWndParent,(DLGPROC)(ControlSizeDlgProc),(LPARAM)pCamera);
}
