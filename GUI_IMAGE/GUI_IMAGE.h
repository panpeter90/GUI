
// GUI_IMAGE.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CGUI_IMAGEApp:
// See GUI_IMAGE.cpp for the implementation of this class
//

class CGUI_IMAGEApp : public CWinApp
{
public:
	CGUI_IMAGEApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CGUI_IMAGEApp theApp;
