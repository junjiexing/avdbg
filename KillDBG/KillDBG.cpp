// KillDBG.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "KillDBG.h"
#include "MainFrm.h"
#include "OutputWindow.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class debug_kernel;
std::shared_ptr<debug_kernel> debug_kernel_ptr;

// CKillDBGApp

BEGIN_MESSAGE_MAP(CKillDBGApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CKillDBGApp::OnAppAbout)
	//	ON_COMMAND(ID_BUTTONDISWND, &CKillDBGApp::OnButtondiswnd)
END_MESSAGE_MAP()


// CKillDBGApp construction

CKillDBGApp::CKillDBGApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CKillDBGApp object

CKillDBGApp theApp;
CMainFrame* main_frame = NULL;

// CKillDBGApp initialization

BOOL CKillDBGApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("KillDBG"));
	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object

	TextView::reg_class();
	//提升自身到 Debug Privilege
	HANDLE hProcess=GetCurrentProcess();
	HANDLE hToken;
	BOOL bRet=FALSE;

	if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		LUID luid;

		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		{
			TOKEN_PRIVILEGES tp;

			tp.PrivilegeCount=1;
			tp.Privileges[0].Luid=luid;
			tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
			{
				bRet=(GetLastError() == ERROR_SUCCESS);
			}
		}
		CloseHandle(hToken);
	}

	if (!bRet)
	{
		MessageBox(NULL,_T("提升进程权限到SE_DEBUG_NAME失败，可能会导致部分进程无法调试"),NULL,MB_OK | MB_ICONWARNING);
	}

	//创建主窗口
	main_frame = new CMainFrame;
	if (!main_frame)
		return FALSE;
	m_pMainWnd = main_frame;
	// create and load the frame with its resources
	main_frame->LoadFrame(IDR_MAINFRAME,WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,NULL);

	// The one and only window has been initialized, so show and update it
	main_frame->ShowWindow(SW_SHOW);
	main_frame->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;
}


// CKillDBGApp message handlers




// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CKillDBGApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CKillDBGApp message handlers



//void CKillDBGApp::OnButtondiswnd()
//{
//	MessageBox(NULL,_T("Dissasmbly"),NULL,MB_OK);
//}


int CKillDBGApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
