//  1394CameraDemo.cpp : Defines the class behaviors for the application.
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

#define ISOLATION_AWARE_ENABLED 1
#define MANIFEST_RESOURCE_ID 2
#include "stdafx.h"
#include "1394CameraDemo.h"
#include "1394CameraDlg.h"
#include "1394PartialScanDlg.h"
#include <1394Camera.h>

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// C1394CameraDemoApp

BEGIN_MESSAGE_MAP(C1394CameraDemoApp, CWinApp)
	//{{AFX_MSG_MAP(C1394CameraDemoApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_1394_15FPS, On139415fps)
	ON_UPDATE_COMMAND_UI(ID_1394_15FPS, OnUpdate139415fps)
	ON_COMMAND(ID_1394_160X120YUV444, On1394160x120yuv444)
	ON_UPDATE_COMMAND_UI(ID_1394_160X120YUV444, OnUpdate1394160x120yuv444)
	ON_COMMAND(ID_1394_30FPS, On139430fps)
	ON_UPDATE_COMMAND_UI(ID_1394_30FPS, OnUpdate139430fps)
	ON_COMMAND(ID_1394_320X240YUV422, On1394320x240yuv422)
	ON_UPDATE_COMMAND_UI(ID_1394_320X240YUV422, OnUpdate1394320x240yuv422)
	ON_COMMAND(ID_1394_4FPS, On13944fps)
	ON_UPDATE_COMMAND_UI(ID_1394_4FPS, OnUpdate13944fps)
	ON_COMMAND(ID_1394_640X480YUV411, On1394640x480yuv411)
	ON_UPDATE_COMMAND_UI(ID_1394_640X480YUV411, OnUpdate1394640x480yuv411)
	ON_COMMAND(ID_1394_640X480YUV422, On1394640x480yuv422)
	ON_UPDATE_COMMAND_UI(ID_1394_640X480YUV422, OnUpdate1394640x480yuv422)
	ON_COMMAND(ID_1394_7FPS, On13947fps)
	ON_UPDATE_COMMAND_UI(ID_1394_7FPS, OnUpdate13947fps)
	ON_COMMAND(ID_1394_CAMERA_MODEL, On1394CameraModel)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA_MODEL, OnUpdate1394CameraModel)
	ON_COMMAND(ID_1394_CHECK_LINK, On1394CheckLink)
	ON_COMMAND(ID_1394_CONTROL, On1394Control)
	ON_UPDATE_COMMAND_UI(ID_1394_CONTROL, OnUpdate1394Control)
	ON_COMMAND(ID_1394_INIT_CAMERA, On1394InitCamera)
	ON_UPDATE_COMMAND_UI(ID_1394_INIT_CAMERA, OnUpdate1394InitCamera)
	ON_COMMAND(ID_1394_MAXIMUM_SPEED, On1394MaximumSpeed)
	ON_UPDATE_COMMAND_UI(ID_1394_MAXIMUM_SPEED, OnUpdate1394MaximumSpeed)
	ON_COMMAND(ID_1394_MEASURE_FRAMERATE1, On1394MeasureFramerate1)
	ON_UPDATE_COMMAND_UI(ID_1394_MEASURE_FRAMERATE1, OnUpdate1394MeasureFramerate1)
	ON_COMMAND(ID_1394_MEASURE_FRAMERATE2, On1394MeasureFramerate2)
	ON_UPDATE_COMMAND_UI(ID_1394_MEASURE_FRAMERATE2, OnUpdate1394MeasureFramerate2)
	ON_UPDATE_COMMAND_UI(ID_1394_RESET_LINK, OnUpdate1394ResetLink)
	ON_COMMAND(ID_1394_SHOW_CAMERA, On1394ShowCamera)
	ON_UPDATE_COMMAND_UI(ID_1394_SHOW_CAMERA, OnUpdate1394ShowCamera)
	ON_COMMAND(ID_1394_SHOW_CAMERA2, On1394ShowCamera2)
	ON_UPDATE_COMMAND_UI(ID_1394_SHOW_CAMERA2, OnUpdate1394ShowCamera2)
	ON_COMMAND(ID_1394_STOP_CAMERA, On1394StopCamera)
	ON_UPDATE_COMMAND_UI(ID_1394_STOP_CAMERA, OnUpdate1394StopCamera)
	ON_UPDATE_COMMAND_UI(ID_APP_EXIT, OnUpdateAppExit)
	ON_COMMAND(ID_1394_1024X768MONO, On13941024x768mono)
	ON_UPDATE_COMMAND_UI(ID_1394_1024X768MONO, OnUpdate13941024x768mono)
	ON_COMMAND(ID_1394_1024X768RGB, On13941024x768rgb)
	ON_UPDATE_COMMAND_UI(ID_1394_1024X768RGB, OnUpdate13941024x768rgb)
	ON_COMMAND(ID_1394_1024X768YUV422, On13941024x768yuv422)
	ON_UPDATE_COMMAND_UI(ID_1394_1024X768YUV422, OnUpdate13941024x768yuv422)
	ON_COMMAND(ID_1394_1280X960MONO, On13941280x960mono)
	ON_UPDATE_COMMAND_UI(ID_1394_1280X960MONO, OnUpdate13941280x960mono)
	ON_COMMAND(ID_1394_1280X960RGB, On13941280x960rgb)
	ON_UPDATE_COMMAND_UI(ID_1394_1280X960RGB, OnUpdate13941280x960rgb)
	ON_COMMAND(ID_1394_1280X960YUV422, On13941280x960yuv422)
	ON_UPDATE_COMMAND_UI(ID_1394_1280X960YUV422, OnUpdate13941280x960yuv422)
	ON_COMMAND(ID_1394_1600X1200MONO, On13941600x1200mono)
	ON_UPDATE_COMMAND_UI(ID_1394_1600X1200MONO, OnUpdate13941600x1200mono)
	ON_COMMAND(ID_1394_1600X1200RGB, On13941600x1200rgb)
	ON_UPDATE_COMMAND_UI(ID_1394_1600X1200RGB, OnUpdate13941600x1200rgb)
	ON_COMMAND(ID_1394_1600X1200YUV422, On13941600x1200yuv422)
	ON_UPDATE_COMMAND_UI(ID_1394_1600X1200YUV422, OnUpdate13941600x1200yuv422)
	ON_COMMAND(ID_1394_640X480MONO, On1394640x480mono)
	ON_UPDATE_COMMAND_UI(ID_1394_640X480MONO, OnUpdate1394640x480mono)
	ON_COMMAND(ID_1394_640X480RGB, On1394640x480rgb)
	ON_UPDATE_COMMAND_UI(ID_1394_640X480RGB, OnUpdate1394640x480rgb)
	ON_COMMAND(ID_1394_800X600MONO, On1394800x600mono)
	ON_UPDATE_COMMAND_UI(ID_1394_800X600MONO, OnUpdate1394800x600mono)
	ON_COMMAND(ID_1394_800X600RGB, On1394800x600rgb)
	ON_UPDATE_COMMAND_UI(ID_1394_800X600RGB, OnUpdate1394800x600rgb)
	ON_COMMAND(ID_1394_800X600YUV422, On1394800x600yuv422)
	ON_UPDATE_COMMAND_UI(ID_1394_800X600YUV422, OnUpdate1394800x600yuv422)
	ON_COMMAND(ID_1394_2FPS, On13942fps)
	ON_UPDATE_COMMAND_UI(ID_1394_2FPS, OnUpdate13942fps)
	ON_COMMAND(ID_1394_60FPS, On139460fps)
	ON_UPDATE_COMMAND_UI(ID_1394_60FPS, OnUpdate139460fps)
	ON_COMMAND(ID_1394_TRIGGER, On1394Trigger)
	ON_UPDATE_COMMAND_UI(ID_1394_TRIGGER, OnUpdate1394Trigger)
	ON_COMMAND(ID_1394_PARTIALSCAN, On1394Partialscan)
	ON_UPDATE_COMMAND_UI(ID_1394_PARTIALSCAN, OnUpdate1394Partialscan)
	ON_COMMAND(ID_1394_CAMERA1, On1394Camera1)
	ON_COMMAND(ID_1394_CAMERA2, On1394Camera2)
	ON_COMMAND(ID_1394_CAMERA3, On1394Camera3)
	ON_COMMAND(ID_1394_CAMERA4, On1394Camera4)
	ON_COMMAND(ID_1394_CAMERA5, On1394Camera5)
	ON_COMMAND(ID_1394_CAMERA6, On1394Camera6)
	ON_COMMAND(ID_1394_CAMERA7, On1394Camera7)
	ON_COMMAND(ID_1394_CAMERA8, On1394Camera8)
	ON_COMMAND(ID_1394_CAMERA9, On1394Camera9)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA1, OnUpdate1394Camera1)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA2, OnUpdate1394Camera2)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA3, OnUpdate1394Camera3)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA4, OnUpdate1394Camera4)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA5, OnUpdate1394Camera5)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA6, OnUpdate1394Camera6)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA7, OnUpdate1394Camera7)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA8, OnUpdate1394Camera8)
	ON_UPDATE_COMMAND_UI(ID_1394_CAMERA9, OnUpdate1394Camera9)
	ON_COMMAND(ID_1394_1024X768MONO16, On13941024x768mono16)
	ON_UPDATE_COMMAND_UI(ID_1394_1024X768MONO16, OnUpdate13941024x768mono16)
	ON_COMMAND(ID_1394_1280X960MONO16, On13941280x960mono16)
	ON_UPDATE_COMMAND_UI(ID_1394_1280X960MONO16, OnUpdate13941280x960mono16)
	ON_COMMAND(ID_1394_1600X1200MONO16, On13941600x1200mono16)
	ON_UPDATE_COMMAND_UI(ID_1394_1600X1200MONO16, OnUpdate13941600x1200mono16)
	ON_COMMAND(ID_1394_640X480MONO16, On1394640x480mono16)
	ON_UPDATE_COMMAND_UI(ID_1394_640X480MONO16, OnUpdate1394640x480mono16)
	ON_COMMAND(ID_1394_800X600MONO16, On1394800x600mono16)
	ON_UPDATE_COMMAND_UI(ID_1394_800X600MONO16, OnUpdate1394800x600mono16)
	ON_COMMAND(ID_APP_DEBUG, OnAppDebug)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C1394CameraDemoApp construction

C1394CameraDemoApp::C1394CameraDemoApp()
{
	m_showCamera = false;
	m_pBitmap = NULL;
	m_pDlg = NULL;
	m_hThreadDoneEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
}

C1394CameraDemoApp::~C1394CameraDemoApp()
{
	CloseHandle(m_hThreadDoneEvent);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only C1394CameraDemoApp object

C1394CameraDemoApp theApp;
C1394Camera theCamera;


/////////////////////////////////////////////////////////////////////////////
// C1394CameraDemoApp initialization

BOOL C1394CameraDemoApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	//SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources

	HICON hIcon = LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME));

	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	pFrame->SetIcon(hIcon,TRUE);

	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();

	CRect windowRect, clientRect;
	m_pMainWnd->GetWindowRect(&windowRect);
	m_pMainWnd->GetClientRect(&clientRect);
	m_borderWidth = windowRect.Width() - clientRect.Width() + 1;
	m_borderHeight = windowRect.Height() - clientRect.Height() + 20;
	m_pMainWnd->SetWindowPos(NULL, 0, 0, 320+m_borderWidth, 240+m_borderHeight, SWP_NOMOVE|SWP_NOZORDER);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// C1394CameraDemoApp message handlers





/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void C1394CameraDemoApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}



/////////////////////////////////////////////////////////////////////////////
// C1394CameraDemoApp message handlers

void C1394CameraDemoApp::On1394CheckLink() 
{
	char buf[64];
	((CMainFrame *)m_pMainWnd)->SetStatus("Looking For Cameras...");
	if(theCamera.CheckLink())
		AfxMessageBox("No Cameras Found");
	else
	{
		sprintf(buf,"Connection OK, found %d camera(s)",theCamera.GetNumberCameras());
		AfxMessageBox(buf);
	}
	((CMainFrame *)m_pMainWnd)->SetStatus("Ready");
}


//DEL void C1394CameraDemoApp::On1394ResetLink() 
//DEL {
//DEL //	theCamera.ResetLink(true);
//DEL //	theCamera.m_cameraInitialized = false;
//DEL }


void C1394CameraDemoApp::On13942fps() 
{
	theCamera.SetVideoFrameRate(0);
}


void C1394CameraDemoApp::On13944fps() 
{
	theCamera.SetVideoFrameRate(1);
}


void C1394CameraDemoApp::On13947fps() 
{
	theCamera.SetVideoFrameRate(2);
}


void C1394CameraDemoApp::On139415fps() 
{
	theCamera.SetVideoFrameRate(3);
}


void C1394CameraDemoApp::On139430fps() 
{
	theCamera.SetVideoFrameRate(4);
}


void C1394CameraDemoApp::On139460fps() 
{
	theCamera.SetVideoFrameRate(5);
}


void C1394CameraDemoApp::On1394160x120yuv444() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(0);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394320x240yuv422() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(1);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394640x480yuv411() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(2);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394640x480yuv422() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(3);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394640x480rgb() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(4);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394640x480mono() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(5);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394800x600yuv422() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(0);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394800x600rgb() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(1);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On1394800x600mono() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(2);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941024x768yuv422() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(3);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941024x768rgb() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(4);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941024x768mono() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(5);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941280x960yuv422() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(0);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941280x960rgb() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(1);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941280x960mono() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(2);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941600x1200yuv422() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(3);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941600x1200rgb() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(4);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::On13941600x1200mono() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(5);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}


void C1394CameraDemoApp::OnUpdate13942fps(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[theCamera.GetVideoFormat()][theCamera.GetVideoMode()][0]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck(theCamera.GetVideoFrameRate() == 0);
}


void C1394CameraDemoApp::OnUpdate13944fps(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[theCamera.GetVideoFormat()][theCamera.GetVideoMode()][1]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck(theCamera.GetVideoFrameRate() == 1);
}


void C1394CameraDemoApp::OnUpdate13947fps(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[theCamera.GetVideoFormat()][theCamera.GetVideoMode()][2]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck(theCamera.GetVideoFrameRate() == 2);
}


void C1394CameraDemoApp::OnUpdate139415fps(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[theCamera.GetVideoFormat()][theCamera.GetVideoMode()][3]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck(theCamera.GetVideoFrameRate() == 3);
}


void C1394CameraDemoApp::OnUpdate139430fps(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[theCamera.GetVideoFormat()][theCamera.GetVideoMode()][4]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck(theCamera.GetVideoFrameRate() == 4);
}


void C1394CameraDemoApp::OnUpdate139460fps(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[theCamera.GetVideoFormat()][theCamera.GetVideoMode()][5]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck(theCamera.GetVideoFrameRate() == 5);
}


void C1394CameraDemoApp::OnUpdate1394160x120yuv444(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][0][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==0));
}


void C1394CameraDemoApp::OnUpdate1394320x240yuv422(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][1][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==1));
}


void C1394CameraDemoApp::OnUpdate1394640x480yuv411(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][2][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==2));
}


void C1394CameraDemoApp::OnUpdate1394640x480yuv422(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][3][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==3));
}


void C1394CameraDemoApp::OnUpdate1394640x480rgb(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][4][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==4));
}


void C1394CameraDemoApp::OnUpdate1394640x480mono(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][5][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==5));
}


void C1394CameraDemoApp::OnUpdate1394800x600yuv422(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][0][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==0));
}


void C1394CameraDemoApp::OnUpdate1394800x600rgb(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][1][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==1));
}


void C1394CameraDemoApp::OnUpdate1394800x600mono(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][2][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==2));
}


void C1394CameraDemoApp::OnUpdate13941024x768yuv422(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][3][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==3));
}


void C1394CameraDemoApp::OnUpdate13941024x768rgb(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][4][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==4));
}


void C1394CameraDemoApp::OnUpdate13941024x768mono(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][5][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==5));
}


void C1394CameraDemoApp::OnUpdate13941280x960yuv422(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][0][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==0));
}


void C1394CameraDemoApp::OnUpdate13941280x960rgb(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][1][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==1));
}


void C1394CameraDemoApp::OnUpdate13941280x960mono(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][2][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==2));
}


void C1394CameraDemoApp::OnUpdate13941600x1200yuv422(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][3][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==3));
}


void C1394CameraDemoApp::OnUpdate13941600x1200rgb(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][4][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==4));
}


void C1394CameraDemoApp::OnUpdate13941600x1200mono(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][5][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==5));
}

UINT DisplayThreadMethod1(LPVOID pParam)
{
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = theCamera.m_width;
	bmi.bmiHeader.biHeight = theCamera.m_height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 1000;
	bmi.bmiHeader.biYPelsPerMeter = 1000;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	CMainFrame* pWnd = (CMainFrame *) theApp.GetMainWnd();
	RECT rect;
	HDC hDC = GetDC(pWnd->m_hWnd);
	int x,y,w,h,i=0;
	char buf[64];
	unsigned long hist[16], sum = 0, t;
	float fps;

	ResetEvent(theApp.m_hThreadDoneEvent);

	while (theApp.m_showCamera)
	{
		t = clock();
		if (theCamera.AcquireImage())
			AfxMessageBox("Problem Acquiring Image");

		pWnd->GetWindowRect(&rect);
		h = rect.bottom - rect.top - theApp.m_borderHeight;
		w = rect.right - rect.left - theApp.m_borderWidth;

		x = w - theCamera.m_width;
		x >>= 1;
		if(x < 0) x = 0;

		y = h - theCamera.m_height;
		y >>= 1;
		if(y < 0) y = 0;

		theCamera.getDIB(theApp.m_pBitmap);
					
		SetDIBitsToDevice(hDC, x, y, theCamera.m_width, theCamera.m_height, 0, 0, 0, theCamera.m_height, theApp.m_pBitmap, &bmi, DIB_RGB_COLORS);

		if(i > 15)
			sum -= hist[i & 15];

		hist[i & 15] = clock() - t;
		sum += hist[i & 15];

		fps = ( i >= 15 ? 16.0f : (float) (i + 1)) / (float) sum;
/*
		// time lapse
	    // record every 45th frame @ 15 fps-> 20 frames per minute = 1200 frames per hour = 1200 * 225K per hour = 262 MB/hour

		if((i % 45) == 0)
		{
			FILE *fp;

			sprintf(buf,"d:\\timelapse\\frame%06d.ppm",i/100);
			if(fp = fopen(buf,"wb"))
			{
				fprintf(fp,"P6\n%d\n%d\n255\n",theCamera.m_width,theCamera.m_height);
				theCamera.getRGB(theApp.m_pBitmap);
				fwrite(theApp.m_pBitmap,1,theCamera.m_width * theCamera.m_height * 3, fp);
				fclose(fp);
			} else {
				OutputDebugString("Error opening frame file\n");
			}

		}
*/
		sprintf(buf,"Displaying: %2.2f fps",1000.0f * fps);
		pWnd->SetStatus(buf);
		i++;
	}

	if (theCamera.StopImageAcquisition())
		AfxMessageBox("Problem Stopping Image Acquisition");

	delete [] theApp.m_pBitmap;
	theApp.m_pBitmap = NULL;
	pWnd->SetStatus("Ready");

	SetEvent(theApp.m_hThreadDoneEvent);

	return (0);
}


void C1394CameraDemoApp::On1394ShowCamera() 
{
	if (theCamera.StartImageAcquisition())
		AfxMessageBox("Problem Starting Image Acquisition");
	else
	{
		m_showCamera = true;
		m_pBitmap = new unsigned char [theCamera.m_width * theCamera.m_height * 3];
		AfxBeginThread(DisplayThreadMethod1, NULL);
	}
}


UINT DisplayThreadMethod2(LPVOID pParam)
{
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = theCamera.m_width;
	bmi.bmiHeader.biHeight = theCamera.m_height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 1000;
	bmi.bmiHeader.biYPelsPerMeter = 1000;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	CMainFrame* pWnd = (CMainFrame *) theApp.GetMainWnd();
	RECT rect;
	HDC hDC = GetDC(pWnd->m_hWnd);
	int x,y,w,h,i=0;
	char buf[64];
	unsigned long hist[16], sum = 0, t;
	float fps;

	ResetEvent(theApp.m_hThreadDoneEvent);

	while (theApp.m_showCamera)
	{
		t = clock();
   		if (theCamera.CaptureImage())
			AfxMessageBox("Problem Capturing Image");

		pWnd->GetWindowRect(&rect);
		h = rect.bottom - rect.top - theApp.m_borderHeight;
		w = rect.right - rect.left - theApp.m_borderWidth;

		x = w - theCamera.m_width;
		x >>= 1;
		if(x < 0) x = 0;

		y = h - theCamera.m_height;
		y >>= 1;
		if(y < 0) y = 0;

		theCamera.getDIB(theApp.m_pBitmap);
					
		SetDIBitsToDevice(hDC, x, y, theCamera.m_width, theCamera.m_height, 0, 0, 0, theCamera.m_height, theApp.m_pBitmap, &bmi, DIB_RGB_COLORS);
		//theCamera.m_pCycleTime->CL_CycleOffset = 9;

		if(i > 15)
			sum -= hist[i & 15];

		hist[i & 15] = clock() - t;
		sum += hist[i & 15];

		fps = ( i >= 15 ? 16.0f : (float) (i + 1)) / (float) sum;
		
		sprintf(buf,"Displaying: %2.2f fps",1000.0f * fps);
		pWnd->SetStatus(buf);
		i++;
	}

	if (theCamera.StopImageCapture())
		AfxMessageBox("Problem Stopping Image Capture");

	delete [] theApp.m_pBitmap;
	theApp.m_pBitmap = NULL;

	SetEvent(theApp.m_hThreadDoneEvent);
	
	return (0);
}


void C1394CameraDemoApp::On1394ShowCamera2() 
{
	if (theCamera.StartImageCapture())
		AfxMessageBox("Problem Starting Image Capture");
	else
	{
		m_showCamera = true;
		m_pBitmap = new unsigned char [theCamera.m_width * theCamera.m_height * 3];
		AfxBeginThread(DisplayThreadMethod2, NULL);
	}
}


void C1394CameraDemoApp::On1394StopCamera() 
{
	m_showCamera = false;
	WaitForSingleObject(theApp.m_hThreadDoneEvent,1000);
}


void C1394CameraDemoApp::OnUpdate1394ShowCamera(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_cameraInitialized);
}


void C1394CameraDemoApp::OnUpdate1394ShowCamera2(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_cameraInitialized);
}


void C1394CameraDemoApp::OnUpdate1394StopCamera(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_showCamera);
}


void C1394CameraDemoApp::On1394MeasureFramerate1() 
{
	CString text;
	DWORD start, duration;
	int frames = 0;

	if (theCamera.StartImageAcquisition())
		AfxMessageBox("Problem Starting Image Acquisition");

	start = GetTickCount();
	do
	{
   		if (theCamera.AcquireImage())
			AfxMessageBox("Problem Acquiring Image");
		frames++;
		duration = GetTickCount() - start;
	}
	while (duration < 3000);

	if (theCamera.StopImageAcquisition())
		AfxMessageBox("Problem Stopping Image Acquisition");

	double rate = (1000.0*frames)/duration;
	text.Format("Frames = %d; Rate = %3.1f", frames, rate);
	AfxMessageBox(text);
}


void C1394CameraDemoApp::On1394MeasureFramerate2() 
{
	CString text;
	DWORD start, duration;
	int frames = 0;

	if (theCamera.StartImageCapture())
		AfxMessageBox("Problem Starting Image Capture");

	start = GetTickCount();
	do
	{
   		if (theCamera.CaptureImage())
			AfxMessageBox("Problem Capturing Image");
		frames++;
		duration = GetTickCount() - start;
	}
	while (duration < 3000);

	if (theCamera.StopImageCapture())
		AfxMessageBox("Problem Stopping Image Capture");

	double rate = (1000.0*frames)/duration;
	text.Format("Frames = %d; Rate = %3.1f", frames, rate);
	AfxMessageBox(text);
}


void C1394CameraDemoApp::OnUpdate1394MeasureFramerate1(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_cameraInitialized);
}


void C1394CameraDemoApp::OnUpdate1394MeasureFramerate2(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_cameraInitialized);
}


void C1394CameraDemoApp::On1394InitCamera() 
{
	((CMainFrame *)m_pMainWnd)->SetStatus("Initializing Camera...");
	theCamera.InitCamera();
	((CMainFrame *)m_pMainWnd)->SetStatus("Checking Feature Presence...");
	theCamera.InquireControlRegisters();
	((CMainFrame *)m_pMainWnd)->SetStatus("Checking Feature Status...");
	theCamera.StatusControlRegisters();

	((CMainFrame *)m_pMainWnd)->SetStatus("Setting Initial Video Mode...");
	// set an initial video format, mode, and frame rate
	for (int format=0; format<3; format++)
		for (int mode=0; mode<8; mode++)
			for (int rate=0; rate<6; rate++)
				if (theCamera.m_videoFlags[format][mode][rate])
				{
					theCamera.SetVideoFormat(format);
					theCamera.SetVideoMode(mode);
					theCamera.SetVideoFrameRate(rate);
					m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth + 12, theCamera.m_height+m_borderHeight + 12, SWP_NOMOVE|SWP_NOZORDER);
					//if (!((format==0)&&(mode==0)&&(rate==2)))		// this combination doesn't work (bug somewhere in the code)
					//{
						((CMainFrame *)m_pMainWnd)->SetStatus("Ready");
						return;
					//}
				}

	((CMainFrame *)m_pMainWnd)->SetStatus("Could not find a valid video mode!");

}


void C1394CameraDemoApp::OnUpdate1394InitCamera(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked);
}


void C1394CameraDemoApp::On1394CameraModel() 
{
	char buf[256];
	sprintf(buf,"Vendor: %s\r\nModel: %s\r\nUniqueID: %08x%08x",
		theCamera.m_nameVendor,
		theCamera.m_nameModel,
		theCamera.m_UniqueID.LowPart,
		theCamera.m_UniqueID.HighPart);

	MessageBox(m_pMainWnd->GetSafeHwnd(),buf,"1394Camera Identification",MB_OK);

}


void C1394CameraDemoApp::OnUpdate1394CameraModel(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theCamera.m_cameraInitialized);
}


void C1394CameraDemoApp::On1394Control() 
{
	CameraControlDialog(m_pMainWnd->GetSafeHwnd(),&theCamera,TRUE);
}


void C1394CameraDemoApp::OnUpdate1394Control(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theCamera.m_cameraInitialized);
}


void C1394CameraDemoApp::OnUpdateAppExit(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera);
}


void C1394CameraDemoApp::OnUpdate1394ResetLink(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera);
}


void C1394CameraDemoApp::On1394MaximumSpeed() 
{
	CString text;
	text.Format("Maximum Speed: %d MBits/s", theCamera.GetMaxSpeed());	
	AfxMessageBox(text);
}


void C1394CameraDemoApp::OnUpdate1394MaximumSpeed(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theCamera.m_cameraInitialized);
}

void C1394CameraDemoApp::On1394Trigger()
{
	theCamera.m_controlTrigger.SetMode(0, 0);
	theCamera.m_controlTrigger.Status();
	if (theCamera.m_controlTrigger.m_statusOnOff)
		theCamera.m_controlTrigger.TurnOn(false);
	else
		theCamera.m_controlTrigger.TurnOn(true);
}


void C1394CameraDemoApp::OnUpdate1394Trigger(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theCamera.m_cameraInitialized && theCamera.m_controlTrigger.m_present);
	pCmdUI->SetCheck(theCamera.m_controlTrigger.m_statusOnOff);
}


void C1394CameraDemoApp::On1394Partialscan() 
{
	int i;
	int oldfmt = theCamera.GetVideoFormat();
	int oldmod = theCamera.GetVideoMode();

	theCamera.SetVideoFormat(7);

	// pick an available partial scan mode
	for (i=0; i<8; i++)
		if (theCamera.m_controlSize.ModeSupported(i))
			theCamera.SetVideoMode(i);

	i = CameraControlSizeDialog(GetMainWnd()->GetSafeHwnd(), 
						&theCamera);
	
	if(i == IDOK)
	{
		m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
	} else {
		theCamera.SetVideoFormat(oldfmt);
		theCamera.SetVideoMode(oldmod);
	}
}


void C1394CameraDemoApp::OnUpdate1394Partialscan(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_cameraInitialized && theCamera.m_bxAvailableFormats[7]);
	pCmdUI->SetCheck(theCamera.GetVideoFormat() == 7);
}


void C1394CameraDemoApp::On1394Camera1() 
{
	theCamera.SelectCamera(0);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera2() 
{
	theCamera.SelectCamera(1);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera3() 
{
	theCamera.SelectCamera(2);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera4() 
{
	theCamera.SelectCamera(3);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera5() 
{
	theCamera.SelectCamera(4);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera6() 
{
	theCamera.SelectCamera(5);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera7() 
{
	theCamera.SelectCamera(6);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera8() 
{
	theCamera.SelectCamera(7);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::On1394Camera9() 
{
	theCamera.SelectCamera(8);
	theCamera.m_cameraInitialized = false;
}


void C1394CameraDemoApp::OnUpdate1394Camera1(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 1));
	pCmdUI->SetCheck(theCamera.GetNode() == 0);
}


void C1394CameraDemoApp::OnUpdate1394Camera2(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 2));
	pCmdUI->SetCheck(theCamera.GetNode() == 1);
}


void C1394CameraDemoApp::OnUpdate1394Camera3(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 3));
	pCmdUI->SetCheck(theCamera.GetNode() == 2);
}


void C1394CameraDemoApp::OnUpdate1394Camera4(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 4));
	pCmdUI->SetCheck(theCamera.GetNode() == 3);
}


void C1394CameraDemoApp::OnUpdate1394Camera5(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 5));
	pCmdUI->SetCheck(theCamera.GetNode() == 4);
}


void C1394CameraDemoApp::OnUpdate1394Camera6(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 6));
	pCmdUI->SetCheck(theCamera.GetNode() == 5);
}


void C1394CameraDemoApp::OnUpdate1394Camera7(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 7));
	pCmdUI->SetCheck(theCamera.GetNode() == 6);
}


void C1394CameraDemoApp::OnUpdate1394Camera8(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 8));
	pCmdUI->SetCheck(theCamera.GetNode() == 7);
}


void C1394CameraDemoApp::OnUpdate1394Camera9(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_showCamera && theCamera.m_linkChecked && (theCamera.GetNumberCameras() >= 9));
	pCmdUI->SetCheck(theCamera.GetNode() == 8);
}

// below here added on 2-26-2002 for 16-bit mono modes

void C1394CameraDemoApp::On1394640x480mono16() 
{
	theCamera.SetVideoFormat(0);
	theCamera.SetVideoMode(6);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}

void C1394CameraDemoApp::On1394800x600mono16() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(6);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}

void C1394CameraDemoApp::On13941024x768mono16() 
{
	theCamera.SetVideoFormat(1);
	theCamera.SetVideoMode(7);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}

void C1394CameraDemoApp::On13941280x960mono16() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(6);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}

void C1394CameraDemoApp::On13941600x1200mono16() 
{
	theCamera.SetVideoFormat(2);
	theCamera.SetVideoMode(7);
	m_pMainWnd->SetWindowPos(NULL, 0, 0, theCamera.m_width+m_borderWidth+12, theCamera.m_height+m_borderHeight+12, SWP_NOMOVE|SWP_NOZORDER);
}

void C1394CameraDemoApp::OnUpdate1394640x480mono16(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[0][6][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==0)&&(theCamera.GetVideoMode()==6));
}

void C1394CameraDemoApp::OnUpdate1394800x600mono16(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][6][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==6));
}

void C1394CameraDemoApp::OnUpdate13941024x768mono16(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[1][7][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==1)&&(theCamera.GetVideoMode()==7));
}

void C1394CameraDemoApp::OnUpdate13941280x960mono16(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][6][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==6));
}

void C1394CameraDemoApp::OnUpdate13941600x1200mono16(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((theCamera.m_videoFlags[2][7][theCamera.GetVideoFrameRate()]) && !m_showCamera && theCamera.m_cameraInitialized);
	pCmdUI->SetCheck((theCamera.GetVideoFormat()==2)&&(theCamera.GetVideoMode()==7));
}

int C1394CameraDemoApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
	m_showCamera = false;
	WaitForSingleObject(theApp.m_hThreadDoneEvent,1000);
	return CWinApp::ExitInstance();
}

void C1394CameraDemoApp::OnAppDebug() 
{
	// TODO: Add your command handler code here
	CameraDebugDialog(GetMainWnd()->GetSafeHwnd());	
}
