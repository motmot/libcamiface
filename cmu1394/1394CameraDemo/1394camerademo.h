// 1394CameraDemo.h : main header file for the 1394CAMERADEMO application
//
//	Version 4.1
//
//	Copyright 6/2000
// 
//	Iwan Ulrich
//	Robotics Institute
//	Carnegie Mellon University
//	Pittsburgh, PA
//

#if !defined(AFX_1394CAMERADEMO_H__08CE0810_9B9C_11D3_98EB_C7E0B1CE837D__INCLUDED_)
#define AFX_1394CAMERADEMO_H__08CE0810_9B9C_11D3_98EB_C7E0B1CE837D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

class C1394CameraDlg;
class C1394Camera;
extern C1394Camera theCamera;
class C1394CameraDemoApp;
extern C1394CameraDemoApp theApp;


/////////////////////////////////////////////////////////////////////////////
// C1394CameraDemoApp:
// See 1394CameraDemo.cpp for the implementation of this class
//

class C1394CameraDemoApp : public CWinApp
{
public:
	C1394CameraDemoApp();
	~C1394CameraDemoApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C1394CameraDemoApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	bool m_showCamera;
	HANDLE m_hThreadDoneEvent;
	unsigned char *m_pBitmap;
	int m_borderWidth;
	int m_borderHeight;
	C1394CameraDlg* m_pDlg;

	//{{AFX_MSG(C1394CameraDemoApp)
	afx_msg void OnAppAbout();
	afx_msg void On139415fps();
	afx_msg void OnUpdate139415fps(CCmdUI* pCmdUI);
	afx_msg void On1394160x120yuv444();
	afx_msg void OnUpdate1394160x120yuv444(CCmdUI* pCmdUI);
	afx_msg void On139430fps();
	afx_msg void OnUpdate139430fps(CCmdUI* pCmdUI);
	afx_msg void On1394320x240yuv422();
	afx_msg void OnUpdate1394320x240yuv422(CCmdUI* pCmdUI);
	afx_msg void On13944fps();
	afx_msg void OnUpdate13944fps(CCmdUI* pCmdUI);
	afx_msg void On1394640x480yuv411();
	afx_msg void OnUpdate1394640x480yuv411(CCmdUI* pCmdUI);
	afx_msg void On1394640x480yuv422();
	afx_msg void OnUpdate1394640x480yuv422(CCmdUI* pCmdUI);
	afx_msg void On13947fps();
	afx_msg void OnUpdate13947fps(CCmdUI* pCmdUI);
	afx_msg void On1394CameraModel();
	afx_msg void OnUpdate1394CameraModel(CCmdUI* pCmdUI);
	afx_msg void On1394CheckLink();
	afx_msg void On1394Control();
	afx_msg void OnUpdate1394Control(CCmdUI* pCmdUI);
	afx_msg void On1394InitCamera();
	afx_msg void OnUpdate1394InitCamera(CCmdUI* pCmdUI);
	afx_msg void On1394MaximumSpeed();
	afx_msg void OnUpdate1394MaximumSpeed(CCmdUI* pCmdUI);
	afx_msg void On1394MeasureFramerate1();
	afx_msg void OnUpdate1394MeasureFramerate1(CCmdUI* pCmdUI);
	afx_msg void On1394MeasureFramerate2();
	afx_msg void OnUpdate1394MeasureFramerate2(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394ResetLink(CCmdUI* pCmdUI);
	afx_msg void On1394ShowCamera();
	afx_msg void OnUpdate1394ShowCamera(CCmdUI* pCmdUI);
	afx_msg void On1394ShowCamera2();
	afx_msg void OnUpdate1394ShowCamera2(CCmdUI* pCmdUI);
	afx_msg void On1394StopCamera();
	afx_msg void OnUpdate1394StopCamera(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAppExit(CCmdUI* pCmdUI);
	afx_msg void On13941024x768mono();
	afx_msg void OnUpdate13941024x768mono(CCmdUI* pCmdUI);
	afx_msg void On13941024x768rgb();
	afx_msg void OnUpdate13941024x768rgb(CCmdUI* pCmdUI);
	afx_msg void On13941024x768yuv422();
	afx_msg void OnUpdate13941024x768yuv422(CCmdUI* pCmdUI);
	afx_msg void On13941280x960mono();
	afx_msg void OnUpdate13941280x960mono(CCmdUI* pCmdUI);
	afx_msg void On13941280x960rgb();
	afx_msg void OnUpdate13941280x960rgb(CCmdUI* pCmdUI);
	afx_msg void On13941280x960yuv422();
	afx_msg void OnUpdate13941280x960yuv422(CCmdUI* pCmdUI);
	afx_msg void On13941600x1200mono();
	afx_msg void OnUpdate13941600x1200mono(CCmdUI* pCmdUI);
	afx_msg void On13941600x1200rgb();
	afx_msg void OnUpdate13941600x1200rgb(CCmdUI* pCmdUI);
	afx_msg void On13941600x1200yuv422();
	afx_msg void OnUpdate13941600x1200yuv422(CCmdUI* pCmdUI);
	afx_msg void On1394640x480mono();
	afx_msg void OnUpdate1394640x480mono(CCmdUI* pCmdUI);
	afx_msg void On1394640x480rgb();
	afx_msg void OnUpdate1394640x480rgb(CCmdUI* pCmdUI);
	afx_msg void On1394800x600mono();
	afx_msg void OnUpdate1394800x600mono(CCmdUI* pCmdUI);
	afx_msg void On1394800x600rgb();
	afx_msg void OnUpdate1394800x600rgb(CCmdUI* pCmdUI);
	afx_msg void On1394800x600yuv422();
	afx_msg void OnUpdate1394800x600yuv422(CCmdUI* pCmdUI);
	afx_msg void On13942fps();
	afx_msg void OnUpdate13942fps(CCmdUI* pCmdUI);
	afx_msg void On139460fps();
	afx_msg void OnUpdate139460fps(CCmdUI* pCmdUI);
	afx_msg void On1394Trigger();
	afx_msg void OnUpdate1394Trigger(CCmdUI* pCmdUI);
	afx_msg void On1394Partialscan();
	afx_msg void OnUpdate1394Partialscan(CCmdUI* pCmdUI);
	afx_msg void On1394Camera1();
	afx_msg void On1394Camera2();
	afx_msg void On1394Camera3();
	afx_msg void On1394Camera4();
	afx_msg void On1394Camera5();
	afx_msg void On1394Camera6();
	afx_msg void On1394Camera7();
	afx_msg void On1394Camera8();
	afx_msg void On1394Camera9();
	afx_msg void OnUpdate1394Camera1(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera2(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera3(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera4(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera5(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera6(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera7(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera8(CCmdUI* pCmdUI);
	afx_msg void OnUpdate1394Camera9(CCmdUI* pCmdUI);
	afx_msg void On13941024x768mono16();
	afx_msg void OnUpdate13941024x768mono16(CCmdUI* pCmdUI);
	afx_msg void On13941280x960mono16();
	afx_msg void OnUpdate13941280x960mono16(CCmdUI* pCmdUI);
	afx_msg void On13941600x1200mono16();
	afx_msg void OnUpdate13941600x1200mono16(CCmdUI* pCmdUI);
	afx_msg void On1394640x480mono16();
	afx_msg void OnUpdate1394640x480mono16(CCmdUI* pCmdUI);
	afx_msg void On1394800x600mono16();
	afx_msg void OnUpdate1394800x600mono16(CCmdUI* pCmdUI);
	afx_msg void OnCameraRegisters();
	afx_msg void OnUpdateCameraRegisters(CCmdUI* pCmdUI);
	afx_msg void OnAppDebug();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_1394CAMERADEMO_H__08CE0810_9B9C_11D3_98EB_C7E0B1CE837D__INCLUDED_)
