#if !defined(AFX_1394PARTIALSCANDLG_H__ACCFA740_0A48_11D4_98EB_B6D3A8C97401__INCLUDED_)
#define AFX_1394PARTIALSCANDLG_H__ACCFA740_0A48_11D4_98EB_B6D3A8C97401__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//  1394PartialScanDlg.h : header file
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
// C1394PartialScanDlg dialog

class C1394PartialScanDlg : public CDialog
{
// Construction
public:
	C1394PartialScanDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(C1394PartialScanDlg)
	enum { IDD = IDD_1394_PARTIAL_SCAN_DIALOG };
	CSpinButtonCtrl	m_spinTop;
	CSpinButtonCtrl	m_spinRight;
	CSpinButtonCtrl	m_spinLeft;
	CSpinButtonCtrl	m_spinBottom;
	int		m_left;
	int		m_right;
	int		m_top;
	int		m_bottom;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C1394PartialScanDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(C1394PartialScanDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnUpdateEditLeft();
	afx_msg void OnUpdateEditRight();
	afx_msg void OnUpdateEditTop();
	afx_msg void OnUpdateEditBottom();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	bool m_initialized;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_1394PARTIALSCANDLG_H__ACCFA740_0A48_11D4_98EB_B6D3A8C97401__INCLUDED_)
