//  1394CameraDlg.cpp : Camera Dialog.
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
#include "1394CameraDemo.h"
#include <1394Camera.h>
#include "1394CameraDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// C1394CameraDlg dialog


C1394CameraDlg::C1394CameraDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C1394CameraDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(C1394CameraDlg)
	m_autoexposure = 0;
	m_brightness = 0;
	m_focus = 0;
	m_gain = 0;
	m_hue = 0;
	m_saturation = 0;
	m_sharpness = 0;
	m_shutter = 0;
	m_whitebalanceU = 0;
	m_whitebalanceV = 0;
	m_zoom = 0;
	m_checkGain = FALSE;
	m_checkAutoexposure = FALSE;
	m_checkShutter = FALSE;
	m_checkWhitebalance = FALSE;
	m_iris = 0;
	m_checkIris = FALSE;
	//}}AFX_DATA_INIT
}


void C1394CameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C1394CameraDlg)
	DDX_Control(pDX, IDC_SLIDER_IRIS, m_sliderIris);
	DDX_Control(pDX, IDC_COMBO_GAMMA, m_comboGamma);
	DDX_Control(pDX, IDC_SLIDER_ZOOM, m_sliderZoom);
	DDX_Control(pDX, IDC_SLIDER_WBALANCEV, m_sliderWhiteBalanceV);
	DDX_Control(pDX, IDC_SLIDER_WBALANCEU, m_sliderWhiteBalanceU);
	DDX_Control(pDX, IDC_SLIDER_SHUTTER, m_sliderShutter);
	DDX_Control(pDX, IDC_SLIDER_SHARPNESS, m_sliderSharpness);
	DDX_Control(pDX, IDC_SLIDER_SATURATION, m_sliderSaturation);
	DDX_Control(pDX, IDC_SLIDER_HUE, m_sliderHue);
	DDX_Control(pDX, IDC_SLIDER_GAIN, m_sliderGain);
	DDX_Control(pDX, IDC_SLIDER_FOCUS, m_sliderFocus);
	DDX_Control(pDX, IDC_SLIDER_BRIGHTNESS, m_sliderBrightness);
	DDX_Control(pDX, IDC_SLIDER_AUTOEXPOSURE, m_sliderAutoExposure);
	DDX_Text(pDX, IDC_EDIT_AUTOEXPOSURE, m_autoexposure);
	DDX_Text(pDX, IDC_EDIT_BRIGHTNESS, m_brightness);
	DDX_Text(pDX, IDC_EDIT_FOCUS, m_focus);
	DDX_Text(pDX, IDC_EDIT_GAIN, m_gain);
	DDX_Text(pDX, IDC_EDIT_HUE, m_hue);
	DDX_Text(pDX, IDC_EDIT_SATURATION, m_saturation);
	DDX_Text(pDX, IDC_EDIT_SHARPNESS, m_sharpness);
	DDX_Text(pDX, IDC_EDIT_SHUTTER, m_shutter);
	DDX_Text(pDX, IDC_EDIT_WBALANCEU, m_whitebalanceU);
	DDX_Text(pDX, IDC_EDIT_WBALANCEV, m_whitebalanceV);
	DDX_Text(pDX, IDC_EDIT_ZOOM, m_zoom);
	DDX_Check(pDX, IDC_CHECK_GAIN, m_checkGain);
	DDX_Check(pDX, IDC_CHECK_AUTOEXPOSURE, m_checkAutoexposure);
	DDX_Check(pDX, IDC_CHECK_SHUTTER, m_checkShutter);
	DDX_Check(pDX, IDC_CHECK_WBALANCE, m_checkWhitebalance);
	DDX_Text(pDX, IDC_EDIT_IRIS, m_iris);
	DDX_Check(pDX, IDC_CHECK_IRIS, m_checkIris);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(C1394CameraDlg, CDialog)
	//{{AFX_MSG_MAP(C1394CameraDlg)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_CHECK_GAIN, OnCheckGain)
	ON_BN_CLICKED(IDC_CHECK_AUTOEXPOSURE, OnCheckAutoexposure)
	ON_BN_CLICKED(IDC_CHECK_SHUTTER, OnCheckShutter)
	ON_BN_CLICKED(IDC_CHECK_WBALANCE, OnCheckWbalance)
	ON_BN_CLICKED(IDC_BUTTON_WB, OnButtonWBOnePush)
	ON_CBN_SELCHANGE(IDC_COMBO_GAMMA, OnSelchangeComboGamma)
	ON_BN_CLICKED(IDC_BUTTON_WB_READ, OnButtonWbRead)
	ON_BN_CLICKED(IDC_CHECK_IRIS, OnCheckIris)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// C1394CameraDlg message handlers

void C1394CameraDlg::OnOK() 
{
	DestroyWindow();
}


void C1394CameraDlg::PostNcDestroy() 
{
	theApp.m_pDlg = NULL;
	delete this;
	CDialog::PostNcDestroy();
}


void C1394CameraDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CSliderCtrl* pSlider = (CSliderCtrl*) pScrollBar;
	int value = pSlider->GetPos();
	
	if (pSlider == &m_sliderBrightness)
	{
		m_brightness = value;
		theCamera.SetBrightness(value);
	}
	else if (pSlider == &m_sliderAutoExposure)
	{
		m_autoexposure = value;
		theCamera.SetAutoExposure(value);
	}
	else if (pSlider == &m_sliderSharpness)
	{
		m_sharpness = value;
		theCamera.SetSharpness(value);
	}
	else if (pSlider == &m_sliderWhiteBalanceU)
	{
		m_whitebalanceU = value;
		theCamera.SetWhiteBalance(value, m_whitebalanceV);
	}
	else if (pSlider == &m_sliderWhiteBalanceV)
	{
		m_whitebalanceV = value;
		theCamera.SetWhiteBalance(m_whitebalanceU, value);
	}
	else if (pSlider == &m_sliderHue)
	{
		m_hue = value;
		theCamera.SetHue(value);
	}
	else if (pSlider == &m_sliderSaturation)
	{
		m_saturation = value;
		theCamera.SetSaturation(value);
	}
	else if (pSlider == &m_sliderShutter)
	{
		m_shutter = value;
		theCamera.SetShutter(value);
	}
	else if (pSlider == &m_sliderZoom)
	{
		m_zoom = value;
		theCamera.SetZoom(value);
	}
	else if (pSlider == &m_sliderFocus)
	{
		m_focus = value;
		theCamera.SetFocus(value);
	}
	else if (pSlider == &m_sliderIris)
	{
		m_iris = value;
		theCamera.SetIris(value);
	}
	else if (pSlider == &m_sliderGain)
	{
		m_gain = value;
		theCamera.SetGain(value);
	}
		
	UpdateData(false);
	
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


BOOL C1394CameraDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_sliderAutoExposure.SetRange(theCamera.m_controlAutoExposure.m_min, theCamera.m_controlAutoExposure.m_max, false);
	m_sliderWhiteBalanceV.SetRange(theCamera.m_controlWhiteBalance.m_min, theCamera.m_controlWhiteBalance.m_max, false);
	m_sliderWhiteBalanceU.SetRange(theCamera.m_controlWhiteBalance.m_min, theCamera.m_controlWhiteBalance.m_max, false);
	m_sliderShutter.SetRange(theCamera.m_controlShutter.m_min, theCamera.m_controlShutter.m_max, false);
	m_sliderSharpness.SetRange(theCamera.m_controlSharpness.m_min, theCamera.m_controlSharpness.m_max, false);
	m_sliderSaturation.SetRange(theCamera.m_controlSaturation.m_min, theCamera.m_controlSaturation.m_max, false);
	m_sliderHue.SetRange(theCamera.m_controlHue.m_min, theCamera.m_controlHue.m_max, false);
	m_sliderGain.SetRange(theCamera.m_controlGain.m_min, theCamera.m_controlGain.m_max, false);
	m_sliderBrightness.SetRange(theCamera.m_controlBrightness.m_min, theCamera.m_controlBrightness.m_max, false);
	m_sliderZoom.SetRange(theCamera.m_controlZoom.m_min, theCamera.m_controlZoom.m_max, false);
	m_sliderFocus.SetRange(theCamera.m_controlFocus.m_min, theCamera.m_controlFocus.m_max, false);
	m_sliderIris.SetRange(theCamera.m_controlIris.m_min, theCamera.m_controlIris.m_max, false);
	
	m_sliderAutoExposure.SetPos(theCamera.m_controlAutoExposure.m_value1);
	m_sliderWhiteBalanceV.SetPos(theCamera.m_controlWhiteBalance.m_value1);
	m_sliderWhiteBalanceU.SetPos(theCamera.m_controlWhiteBalance.m_value2);
	m_sliderShutter.SetPos(theCamera.m_controlShutter.m_value1);
	m_sliderSharpness.SetPos(theCamera.m_controlSharpness.m_value1);
	m_sliderSaturation.SetPos(theCamera.m_controlSaturation.m_value1);
	m_sliderHue.SetPos(theCamera.m_controlHue.m_value1);
	m_sliderGain.SetPos(theCamera.m_controlGain.m_value1);
	m_sliderBrightness.SetPos(theCamera.m_controlBrightness.m_value1);
	m_sliderZoom.SetPos(theCamera.m_controlZoom.m_value1);
	m_sliderFocus.SetPos(theCamera.m_controlFocus.m_value1);
	m_sliderIris.SetPos(theCamera.m_controlIris.m_value1);

	m_autoexposure = theCamera.m_controlAutoExposure.m_value1;
	m_whitebalanceV = theCamera.m_controlWhiteBalance.m_value1;
	m_whitebalanceU = theCamera.m_controlWhiteBalance.m_value2;
	m_shutter = theCamera.m_controlShutter.m_value1;
	m_sharpness = theCamera.m_controlSharpness.m_value1;
	m_saturation = theCamera.m_controlSaturation.m_value1;
	m_hue = theCamera.m_controlHue.m_value1;
	m_gain = theCamera.m_controlGain.m_value1;
	m_brightness = theCamera.m_controlBrightness.m_value1;
	m_zoom = theCamera.m_controlZoom.m_value1;
	m_focus = theCamera.m_controlFocus.m_value1;
	m_iris = theCamera.m_controlIris.m_value1;
	
	CString str;
	for (int i = theCamera.m_controlGamma.m_min; i <= theCamera.m_controlGamma.m_max; i++)
	{
		str.Format("%d",i);
		m_comboGamma.AddString(str);
	}
	m_comboGamma.SetCurSel(0);

	CButton* pButton;
	pButton = (CButton*) GetDlgItem(IDC_CHECK_SHUTTER);
	pButton->EnableWindow(theCamera.m_controlShutter.m_auto && theCamera.m_controlShutter.m_present);
	pButton = (CButton*) GetDlgItem(IDC_CHECK_GAIN);
	pButton->EnableWindow(theCamera.m_controlGain.m_auto && theCamera.m_controlGain.m_present);
	pButton = (CButton*) GetDlgItem(IDC_CHECK_AUTOEXPOSURE);
	pButton->EnableWindow(theCamera.m_controlAutoExposure.m_onoff && theCamera.m_controlAutoExposure.m_present);
	pButton = (CButton*) GetDlgItem(IDC_CHECK_WBALANCE);
	pButton->EnableWindow(theCamera.m_controlWhiteBalance.m_auto && theCamera.m_controlWhiteBalance.m_present);
	pButton = (CButton*) GetDlgItem(IDC_CHECK_IRIS);
	pButton->EnableWindow(theCamera.m_controlIris.m_auto && theCamera.m_controlIris.m_present);
	pButton = (CButton*) GetDlgItem(IDC_BUTTON_WB);
	pButton->EnableWindow(theCamera.m_controlWhiteBalance.m_onePush && theCamera.m_controlWhiteBalance.m_present);
	pButton = (CButton*) GetDlgItem(IDC_BUTTON_WB_READ);
	pButton->EnableWindow(theCamera.m_controlWhiteBalance.m_readout && theCamera.m_controlWhiteBalance.m_present);
	
	CSliderCtrl* pSlider;
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_SHUTTER);
	pSlider->EnableWindow(theCamera.m_controlShutter.m_manual && theCamera.m_controlShutter.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_GAIN);
	pSlider->EnableWindow(theCamera.m_controlGain.m_manual && theCamera.m_controlGain.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_AUTOEXPOSURE);
	pSlider->EnableWindow(theCamera.m_controlAutoExposure.m_manual && theCamera.m_controlAutoExposure.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_WBALANCEU);
	pSlider->EnableWindow(theCamera.m_controlWhiteBalance.m_manual && theCamera.m_controlWhiteBalance.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_WBALANCEV);
	pSlider->EnableWindow(theCamera.m_controlWhiteBalance.m_manual && theCamera.m_controlWhiteBalance.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_HUE);
	pSlider->EnableWindow(theCamera.m_controlHue.m_manual && theCamera.m_controlHue.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_SATURATION);
	pSlider->EnableWindow(theCamera.m_controlSaturation.m_manual && theCamera.m_controlSaturation.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRIGHTNESS);
	pSlider->EnableWindow(theCamera.m_controlBrightness.m_manual && theCamera.m_controlBrightness.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_SHARPNESS);
	pSlider->EnableWindow(theCamera.m_controlSharpness.m_manual && theCamera.m_controlSharpness.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_ZOOM);
	pSlider->EnableWindow(theCamera.m_controlZoom.m_manual && theCamera.m_controlZoom.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_FOCUS);
	pSlider->EnableWindow(theCamera.m_controlFocus.m_manual && theCamera.m_controlFocus.m_present);
	pSlider = (CSliderCtrl*) GetDlgItem(IDC_SLIDER_IRIS);
	pSlider->EnableWindow(theCamera.m_controlIris.m_manual && theCamera.m_controlIris.m_present);

	UpdateData(false);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void C1394CameraDlg::OnCheckGain() 
{
	UpdateData(true);
	theCamera.m_controlGain.SetAutoMode(m_checkGain);
	if (m_checkGain)
		m_sliderGain.EnableWindow(false);
	else
		m_sliderGain.EnableWindow(true);
}


void C1394CameraDlg::OnCheckAutoexposure() 
{
	UpdateData(true);
	theCamera.m_controlAutoExposure.TurnOn(m_checkAutoexposure);
}


void C1394CameraDlg::OnCheckShutter() 
{
	UpdateData(true);
	theCamera.m_controlShutter.SetAutoMode(m_checkShutter);
	if (m_checkShutter)
		m_sliderShutter.EnableWindow(false);
	else
		m_sliderShutter.EnableWindow(true);
}


void C1394CameraDlg::OnCheckWbalance() 
{
	UpdateData(true);
	theCamera.m_controlWhiteBalance.SetAutoMode(m_checkWhitebalance);
	if (m_checkWhitebalance)
	{
		m_sliderWhiteBalanceU.EnableWindow(false);
		m_sliderWhiteBalanceV.EnableWindow(false);
	}
	else
	{
		m_sliderWhiteBalanceU.EnableWindow(true);
		m_sliderWhiteBalanceV.EnableWindow(true);
	}
}


void C1394CameraDlg::OnCheckIris() 
{
	UpdateData(true);
	theCamera.m_controlIris.SetAutoMode(m_checkIris);
	if (m_checkIris)
		m_sliderIris.EnableWindow(false);
	else
		m_sliderIris.EnableWindow(true);
}


void C1394CameraDlg::OnButtonWBOnePush() 
{
	theCamera.m_controlWhiteBalance.SetOnePush();
}


void C1394CameraDlg::OnSelchangeComboGamma() 
{
	int selection = m_comboGamma.GetCurSel();
	theCamera.SetGamma(theCamera.m_controlGamma.m_min + selection);
}


void C1394CameraDlg::OnButtonWbRead() 
{
	UpdateData(true);
	theCamera.m_controlWhiteBalance.Status();
	m_whitebalanceU = theCamera.m_controlWhiteBalance.m_value2;
	m_whitebalanceV = theCamera.m_controlWhiteBalance.m_value1;
	m_sliderWhiteBalanceU.SetPos(m_whitebalanceU);
	m_sliderWhiteBalanceV.SetPos(m_whitebalanceV);
	theCamera.SetWhiteBalance(m_whitebalanceU, m_whitebalanceV);
	UpdateData(false);
}

