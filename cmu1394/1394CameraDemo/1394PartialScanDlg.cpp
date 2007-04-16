//  1394PartialScanDlg.cpp : Partial Scan Dialog
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

#include "stdafx.h"
#include <1394Camera.h>
#include "1394CameraDemo.h"
#include "1394PartialScanDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// C1394PartialScanDlg dialog


C1394PartialScanDlg::C1394PartialScanDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C1394PartialScanDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(C1394PartialScanDlg)
	m_left = 0;
	m_right = 0;
	m_top = 0;
	m_bottom = 0;
	//}}AFX_DATA_INIT
	m_initialized = false;
}


void C1394PartialScanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C1394PartialScanDlg)
	DDX_Control(pDX, IDC_SPIN_TOP, m_spinTop);
	DDX_Control(pDX, IDC_SPIN_RIGHT, m_spinRight);
	DDX_Control(pDX, IDC_SPIN_LEFT, m_spinLeft);
	DDX_Control(pDX, IDC_SPIN_BOTTOM, m_spinBottom);
	DDX_Text(pDX, IDC_EDIT_LEFT, m_left);
	DDX_Text(pDX, IDC_EDIT_RIGHT, m_right);
	DDX_Text(pDX, IDC_EDIT_TOP, m_top);
	DDX_Text(pDX, IDC_EDIT_BOTTOM, m_bottom);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(C1394PartialScanDlg, CDialog)
	//{{AFX_MSG_MAP(C1394PartialScanDlg)
	ON_EN_UPDATE(IDC_EDIT_LEFT, OnUpdateEditLeft)
	ON_EN_UPDATE(IDC_EDIT_RIGHT, OnUpdateEditRight)
	ON_EN_UPDATE(IDC_EDIT_TOP, OnUpdateEditTop)
	ON_EN_UPDATE(IDC_EDIT_BOTTOM, OnUpdateEditBottom)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C1394PartialScanDlg message handlers

BOOL C1394PartialScanDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	theCamera.m_controlSize.Status();
	
	m_left = theCamera.m_controlSize.m_left;
	m_right = m_left + theCamera.m_controlSize.m_width;
	m_top = theCamera.m_controlSize.m_top;
	m_bottom = theCamera.m_controlSize.m_top + theCamera.m_controlSize.m_height;
/*
	m_left /= theCamera.m_controlSize.m_unitH;
	m_right /= theCamera.m_controlSize.m_unitH;
	m_top /= theCamera.m_controlSize.m_unitV;
	m_bottom /= theCamera.m_controlSize.m_unitV;

	m_spinRight.SetRange(m_left + 1, (theCamera.m_controlSize.m_maxH)/(theCamera.m_controlSize.m_unitH));
	m_spinLeft.SetRange(0, m_right - 1);
	m_spinTop.SetRange(0, m_bottom - 1);
	m_spinBottom.SetRange(m_top + 1, (theCamera.m_controlSize.m_maxV)/(theCamera.m_controlSize.m_unitV));
*/
	m_spinRight.SetRange(m_left + 1, (theCamera.m_controlSize.m_maxH));
	m_spinLeft.SetRange(0, m_right - 1);
	m_spinTop.SetRange(0, m_bottom - 1);
	m_spinBottom.SetRange(m_top + 1, (theCamera.m_controlSize.m_maxV));

	UpdateData(false);
	m_initialized = true;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void C1394PartialScanDlg::OnUpdateEditLeft() 
{
	if (m_initialized)
	{
		UpdateData(true);
//		m_spinRight.SetRange(m_left + 1, (theCamera.m_controlSize.m_maxH)/(theCamera.m_controlSize.m_unitH));
		m_spinRight.SetRange(m_left + 1, theCamera.m_controlSize.m_maxH);
	}
}


void C1394PartialScanDlg::OnUpdateEditRight() 
{
	if (m_initialized)
	{
		UpdateData(true);
		m_spinLeft.SetRange(0, m_right - 1);
	}
}


void C1394PartialScanDlg::OnUpdateEditTop() 
{
	if (m_initialized)
	{
		UpdateData(true);
//		m_spinBottom.SetRange(m_top + 1, (theCamera.m_controlSize.m_maxV)/(theCamera.m_controlSize.m_unitV));
		m_spinBottom.SetRange(m_top + 1, (theCamera.m_controlSize.m_maxV));
	}
}


void C1394PartialScanDlg::OnUpdateEditBottom() 
{
	if (m_initialized)
	{
		UpdateData(true);
		m_spinTop.SetRange(0, m_bottom - 1);
	}
}
