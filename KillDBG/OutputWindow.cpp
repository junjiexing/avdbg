// OutputWindow.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "OutputWindow.h"


// COutputWindow

IMPLEMENT_DYNAMIC(COutputWindow, CWnd)

COutputWindow::COutputWindow()
{

}

COutputWindow::~COutputWindow()
{
}


BEGIN_MESSAGE_MAP(COutputWindow, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// COutputWindow 消息处理程序




int COutputWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	RECT rc = {0};
	GetWindowRect(&rc);
	m_TextView.Create(GetSafeHwnd(),0,0,rc.right,rc.bottom);
	//m_TextView.add_line(std::string("aaaaaaaaaaaaa"),true);

	return 0;
}


void COutputWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RECT rc;
	GetClientRect(&rc);
	::MoveWindow(m_TextView.get_hwnd(),0,0,rc.right,rc.bottom,TRUE);
}
