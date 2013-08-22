// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "KillDBG.h"
#include "MainFrm.h"
#include "DebugKernel.h"
#include "FileOpenDlg.h"
#include "AttachProcessDlg.h"
#include "FollowAddressDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_COMMAND(XTP_ID_CUSTOMIZE, OnCustomize)
	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
	ON_COMMAND(ID_BUTTONDISWND, &CMainFrame::OnButtondiswnd)
	ON_COMMAND(ID_VIEW_REGISTER, &CMainFrame::OnViewRegister)
	ON_COMMAND(ID_VIEW_MEMORY, &CMainFrame::OnViewMemory)
	ON_COMMAND(ID_VIEW_STACK, &CMainFrame::OnViewStack)
	ON_COMMAND(ID_VIEW_OUTPUT, &CMainFrame::OnViewOutput)
	ON_COMMAND(ID_VIEW_PESTRUCT, &CMainFrame::OnViewPEStruct)
	ON_COMMAND(ID_VIEW_MEMORYMAP, &CMainFrame::OnViewMemoryMap)

	
	ON_COMMAND(ID_STEP_IN, &CMainFrame::OnStepIn)
	ON_COMMAND(ID_STEP_OVER, &CMainFrame::OnStepOver)
	ON_COMMAND(ID_RUN, &CMainFrame::OnDebugRun)
	ON_COMMAND(ID_FOLLOW_ADDR, &CMainFrame::OnFollowAddr)
	ON_COMMAND(ID_SET_BREAKPOINT, &CMainFrame::OnSetBreakPoint)
//	ON_WM_DESTROY()
	ON_COMMAND(ID_FILE_OPEN, &CMainFrame::OnFileOpen)
	ON_COMMAND(ID_FILE_ATTACH, &CMainFrame::OnFileAttach)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, &CMainFrame::OnUpdateFileOpen)
	ON_UPDATE_COMMAND_UI(ID_FILE_ATTACH, &CMainFrame::OnUpdateFileAttach)
	ON_MESSAGE(WM_USER_DEBUGSTOP, &CMainFrame::OnDebugStop)
	ON_COMMAND(ID_FILE_STOP, &CMainFrame::OnFileStop)
	ON_UPDATE_COMMAND_UI(ID_FILE_STOP, &CMainFrame::OnUpdateFileStop)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

static UINT uHideCmds[] =
{
	ID_FILE_PRINT,
	ID_FILE_PRINT_PREVIEW,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
	:m_hAcc(NULL)
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}


	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// Initialize the command bars
	if (!InitCommandBars())
		return -1;

	// Get a pointer to the command bars object.
	m_pCommandBars = GetCommandBars();
	if(m_pCommandBars == NULL)
	{
		TRACE0("Failed to create command bars object.\n");
		return -1;      // fail to create
	}

	// Add the menu bar
	CXTPCommandBar* pMenuBar = m_pCommandBars->SetMenu(
		_T("Menu Bar"), IDR_MAINFRAME);
	if(pMenuBar == NULL)
	{
		TRACE0("Failed to create menu bar.\n");
		return -1;      // fail to create
	}

	// Create ToolBar
	CXTPToolBar* pToolBarNoraml = m_pCommandBars->Add(_T("Standard"), xtpBarTop);
	if (!pToolBarNoraml || !pToolBarNoraml->LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;
	}

	CXTPToolBar* pToolBarWnd = m_pCommandBars->Add(_T("Window"), xtpBarTop);
	if (!pToolBarWnd || !pToolBarWnd->LoadToolBar(IDR_TOOLBARWND))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;
	}

	m_pCommandBars->LoadBarState("BarState");

	CXTPPaintManager::SetTheme(xtpThemeVisualStudio2010);

	// Hide array of commands
	//pCommandBars->HideCommands(uHideCmds, _countof(uHideCmds));

	// Set "Always Show Full Menus" option to the FALSE
	//pCommandBars->GetCommandBarsOptions()->bAlwaysShowFullMenus = FALSE;

	//pCommandBars->GetShortcutManager()->SetAccelerators(IDR_MAINFRAME);

	// Load the previous state for toolbars and menus.

	SetupDockPane();
//	LoadCommandBars(_T("CommandBars"));

	ACCEL acc[] = 
	{
		{ FCONTROL | FVIRTKEY, 'G', ID_FOLLOW_ADDR }, 
		{ FVIRTKEY, VK_F7,ID_STEP_IN},
		{ FVIRTKEY,VK_F8,ID_STEP_OVER},
		{ FVIRTKEY,VK_F9,ID_RUN},
		{ FVIRTKEY,VK_F2,ID_SET_BREAKPOINT},
		{ FVIRTKEY,VK_F8,ID_STEP_OVER},
	};
	m_hAcc = CreateAcceleratorTable(acc,sizeof(acc)/sizeof(ACCEL));
	return 0;
}

BOOL CMainFrame::SetupDockPane(void)
{
	// Initialize the docking pane manager and set the
	// initial them for the docking panes.  Do this only after all
	// control bars objects have been created and docked.
	m_paneManager.InstallDockingPanes(this);
	m_paneManager.SetDefaultPaneOptions(xtpPaneHasMenuButton);
	m_paneManager.HideClient(TRUE);		//不显示客户区
	m_paneManager.SetAlphaDockingContext(TRUE);		//显示透明停靠Context
	m_paneManager.SetShowDockingContextStickers(TRUE);		//显示停靠导航
	m_paneManager.SetTheme(xtpPaneThemeVisualStudio2010);	//VS2008风格
	m_paneManager.SetStickyFloatingFrames(TRUE);	//可以边缘吸附

	CRect rectDummy(0,0,200,120);
 	//rectDummy.SetRectEmpty();
	m_wndOutputWnd.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,rectDummy, this, AFX_IDW_PANE_FIRST, 0);
	m_wndOutputWnd.ShowWindow(FALSE);

	m_wndAsmView.Create(NULL,rectDummy,this,0);
	m_wndAsmView.ShowWindow(FALSE);

	m_wndModuleList.Create(rectDummy,this,0);
	m_wndModuleList.ShowWindow(FALSE);

	m_wndBpList.Create(rectDummy,this,0);
	m_wndBpList.ShowWindow(FALSE);

	m_wndRegister.Create(WS_VISIBLE | WS_CHILD,rectDummy,this,0);
	m_wndRegister.ShowWindow(FALSE);

	// Create docking panes.
	CXTPDockingPane* pPaneOutputWnd = m_paneManager.CreatePane(IDR_PANE_OUTPUTWND, rectDummy, xtpPaneDockBottom);
	pPaneOutputWnd->SetTitle("Output Window");
	CXTPDockingPane* pPaneAsmView = m_paneManager.CreatePane(IDR_PANE_DISASMWND, rectDummy, xtpPaneDockTop);
	pPaneAsmView->SetTitle("Asm View");
	CXTPDockingPane* pPaneModuleList = m_paneManager.CreatePane(IDR_PANE_MODULELIST, rectDummy, xtpPaneDockTop);
	pPaneModuleList->SetTitle("Module List");
	CXTPDockingPane* pPaneBpList = m_paneManager.CreatePane(IDR_PANE_BPLIST, rectDummy, xtpPaneDockTop);
	pPaneBpList->SetTitle("BreakPoint List");
	CXTPDockingPane* pPaneRegister = m_paneManager.CreatePane(IDR_PANE_REGISTER, rectDummy, xtpPaneDockTop);
	pPaneRegister->SetTitle("Register");

	// Set the icons for the docking pane tabs.
// 	int nIDIcons[] = {IDR_PANE_REGISTER, IDR_PANE_DISASM};
// 	m_paneManager.SetIcons(IDB_BITMAP_ICONS, nIDIcons,_countof(nIDIcons), RGB(0, 255, 0));

	// Load the previous state for docking panes.
	CXTPDockingPaneLayout layoutNormal(&m_paneManager);
	if (layoutNormal.LoadFromFile("FrameLayout","Default"))
	{
		m_paneManager.SetLayout(&layoutNormal);
	}

// 	if (layoutNormal.Load(_T("NormalLayout")))
// 	{
// 		m_paneManager.SetLayout(&layoutNormal);
// 	}
// 
	return 0;
}

LRESULT CMainFrame::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		CXTPDockingPane* pPane = (CXTPDockingPane*)lParam;
		if (!pPane->IsValid())
		{
			CRect rectDummy;
			switch (pPane->GetID())
			{
			case IDR_PANE_OUTPUTWND:
				pPane->Attach(&m_wndOutputWnd);;
				break;
			case IDR_PANE_DISASMWND:
				pPane->Attach(&m_wndAsmView);
				break;
			case IDR_PANE_MODULELIST:
				pPane->Attach(&m_wndModuleList);
				break;
			case IDR_PANE_BPLIST:
				pPane->Attach(&m_wndBpList);
				break;
			case IDR_PANE_REGISTER:
				pPane->Attach(&m_wndRegister);
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	cs.style |= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnClose()
{
	CXTPDockingPaneLayout layoutNormal(&m_paneManager);
	m_paneManager.GetLayout(&layoutNormal);
	layoutNormal.SaveToFile("FrameLayout","Default");
	m_pCommandBars->SaveBarState("BarState");
	//layoutNormal.Save(_T("NormalLayout"));
 	CFrameWnd::OnClose();
}

void CMainFrame::OnCustomize()
{
	// Get a pointer to the command bars object.
	CXTPCommandBars* pCommandBars = GetCommandBars();
	if(pCommandBars != NULL)
	{
		// Instanciate the customize dialog object.
		CXTPCustomizeSheet dlg(pCommandBars);

		// Add the keyboard page to the customize dialog.
		CXTPCustomizeKeyboardPage pageKeyboard(&dlg);
		dlg.AddPage(&pageKeyboard);
		pageKeyboard.AddCategories(IDR_MAINFRAME);

		// Add the options page to the customize dialog.
		CXTPCustomizeOptionsPage pageOptions(&dlg);
		dlg.AddPage(&pageOptions);

		// Add the commands page to the customize dialog.
		CXTPCustomizeCommandsPage* pCommands = dlg.GetCommandsPage();
		pCommands->AddCategories(IDR_MAINFRAME);

		// Use the command bar manager to initialize the
		// customize dialog.
		pCommands->InsertAllCommandsCategory();
		pCommands->InsertBuiltInMenus(IDR_MAINFRAME);
		pCommands->InsertNewMenuCategory();

		// Dispaly the dialog.
		dlg.DoModal();
	}
}

void CMainFrame::OnButtondiswnd()
{
// 	m_paneManager.ShowPane(IDR_PANE_DISASM);
}

void CMainFrame::OnViewRegister()
{
// 	m_paneManager.ShowPane(IDR_PANE_REGISTER);
}

void CMainFrame::OnViewMemory()
{
// 	m_paneManager.ShowPane(IDR_PANE_MEMORY);
}

void CMainFrame::OnViewStack()
{
// 	m_paneManager.ShowPane(IDR_PANE_STACK);
}

void CMainFrame::OnViewOutput()
{
 	m_paneManager.ShowPane(IDR_PANE_OUTPUTWND);
}

void CMainFrame::OnViewPEStruct()
{
// 	m_paneManager.ShowPane(IDR_PANE_PESTRUCT);
}

void CMainFrame::OnViewMemoryMap()
{
	m_paneManager.ShowPane(IDR_PANE_MEMORYMAP);
}

void CMainFrame::OnFileOpen()
{
	CFileOpenDlg	dlg(this);
	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	debug_kernel_ptr.reset(new debug_kernel());
	debug_kernel_ptr->load_exe(dlg.get_path(),dlg.get_param(),dlg.get_run_dir());
}

void CMainFrame::OnFileAttach()
{
	CAttachProcessDlg	dlg(this);

	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	debug_kernel_ptr.reset(new debug_kernel());
	debug_kernel_ptr->attach_process(dlg.m_dwPID);
}

LRESULT CMainFrame::OnDebugStop( WPARAM wParam, LPARAM lParam )
{
	return FALSE;
}

void CMainFrame::OnUpdateFileOpen(CCmdUI *pCmdUI)
{
}

void CMainFrame::OnUpdateFileAttach(CCmdUI *pCmdUI)
{
	OnUpdateFileOpen(pCmdUI);
}

void CMainFrame::OnFileStop()
{
}

void CMainFrame::OnUpdateFileStop(CCmdUI *pCmdUI)
{
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (TranslateAccelerator(GetSafeHwnd(),m_hAcc,pMsg))
	{
		return TRUE;
	}

	return CXTPFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnStepIn()
{
	if (!debug_kernel_ptr)
	{
		return;
	}
	debug_kernel_ptr->step_in();
}

void CMainFrame::OnStepOver()
{
	if (!debug_kernel_ptr)
	{
		return;
	}
	debug_kernel_ptr->step_over();
}

void CMainFrame::OnFollowAddr()
{
	CFollowAddressDlg dlg;
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	m_wndAsmView.SetTopAddr(dlg.m_dwAddr);
}

void CMainFrame::OnDebugRun()
{
	if (!debug_kernel_ptr)
	{
		return;
	}
	debug_kernel_ptr->continue_debug(DBG_CONTINUE);
}

void CMainFrame::OnSetBreakPoint()
{
	if (!debug_kernel_ptr)
	{
		return;
	}

	debug_kernel::breakpoint_t* bp = debug_kernel_ptr->find_breakpoint_by_address(m_wndAsmView.m_dwSelAddrStart);
	if (bp)
	{
		debug_kernel_ptr->delete_breakpoint(bp->address);
		m_wndAsmView.Invalidate(FALSE);
		return;
	}

	if (!debug_kernel_ptr->add_breakpoint(m_wndAsmView.m_dwSelAddrStart))
	{
		char buffer[50];
		sprintf(buffer,"在地址 %08X 处设置断点失败！",m_wndAsmView.m_dwSelAddrStart);
		m_wndOutputWnd.output_string(std::string(buffer),COutputWindow::OUT_ERROR);
		m_wndAsmView.Invalidate(FALSE);
	}
}
