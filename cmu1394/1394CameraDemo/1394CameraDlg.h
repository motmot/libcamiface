#if !defined(AFX_1394CAMERADLG_H__86AFB640_D7F8_11D3_98EB_B22DEE8F3F77__INCLUDED_)
#define AFX_1394CAMERADLG_H__86AFB640_D7F8_11D3_98EB_B22DEE8F3F77__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//  1394CameraDlg.h
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

/////////////////////////////////////////////////////////////////////////////
// C1394CameraDlg dialog

class C1394CameraDlg : public CDialog
{
// Construction
public:
	C1394CameraDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(C1394CameraDlg)
	enum { IDD = IDD_1394_CAMERA_DIALOG };
	CSliderCtrl	m_sliderIris;
	CComboBox	m_comboGamma;
	CSliderCtrl	m_sliderZoom;
	CSliderCtrl	m_sliderWhiteBalanceV;
	CSliderCtrl	m_sliderWhiteBalanceU;
	CSliderCtrl	m_sliderShutter;
	CSliderCtrl	m_sliderSharpness;
	CSliderCtrl	m_sliderSaturation;
	CSliderCtrl	m_sliderHue;
	CSliderCtrl	m_sliderGain;
	CSliderCtrl	m_sliderFocus;
	CSliderCtrl	m_sliderBrightness;
	CSliderCtrl	m_sliderAutoExposure;
	int		m_autoexposure;
	int		m_brightness;
	int		m_focus;
	int		m_gain;
	int		m_hue;
	int		m_saturation;
	int		m_sharpness;
	int		m_shutter;
	int		m_whitebalanceU;
	int		m_whitebalanceV;
	int		m_zoom;
	BOOL	m_checkGain;
	BOOL	m_checkAutoexposure;
	BOOL	m_checkShutter;
	BOOL	m_checkWhitebalance;
	int		m_iris;
	BOOL	m_checkIris;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C1394CameraDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(C1394CameraDlg)
	virtual void OnOK();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckGain();
	afx_msg void OnCheckAutoexposure();
	afx_msg void OnCheckShutter();
	afx_msg void OnCheckWbalance();
	afx_msg void OnButtonWBOnePush();
	afx_msg void OnSelchangeComboGamma();
	afx_msg void OnButtonWbRead();
	afx_msg void OnCheckIris();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_1394CAMERADLG_H__86AFB640_D7F8_11D3_98EB_B22DEE8F3F77__INCLUDED_)
