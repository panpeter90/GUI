
// GUI_IMAGEDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Resource.h"
#include <opencv\cv.h>
#include <opencv\highgui.h>

// CGUI_IMAGEDlg dialog
class CGUI_IMAGEDlg : public CDialogEx
{
// Construction
public:
	CGUI_IMAGEDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_GUI_IMAGE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	
	CButton LoadImage;
	CComboBox ComboBox1;
	CComboBox ComboBox2;
	CComboBox ComboBox3;
	CComboBox ComboBox4;
	CComboBox ComboBox5;
	CComboBox ComboBox6;
	CStatic StaticCar;
	CStatic StaticTruck;
	CStatic StaticBus;
	CStatic StaticVan;
	CStatic StaticContainer;
	
	afx_msg void OnCbnSelchangeCombo2();
	CStatic Picture1;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRec();
	afx_msg void OnBnClickedCheck1();
	CButton CheckBox;
	CButton CheckBoxDebug;
	afx_msg void OnCbnSelchangeCombo6();
	afx_msg void OnBnClickedStart();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnCbnSelchangeCombo4();
	afx_msg void OnCbnSelchangeCombo5();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedCheck2();
};

void ImageDisplay(cv::Mat src_);

