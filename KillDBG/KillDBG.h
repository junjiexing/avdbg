// KillDBG.h : main header file for the KillDBG application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CKillDBGApp:
// See KillDBG.cpp for the implementation of this class
//

class CKillDBGApp : public CWinApp
{
public:
	CKillDBGApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
//	afx_msg void OnButtondiswnd();
	virtual int ExitInstance();
};

extern CKillDBGApp theApp;