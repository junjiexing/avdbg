// ScintillaWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "ScintillaWnd.h"


// CScintillaWnd

IMPLEMENT_DYNAMIC(CScintillaWnd, CWnd)

CScintillaWnd::CScintillaWnd()
{

}

CScintillaWnd::~CScintillaWnd()
{
}


BEGIN_MESSAGE_MAP(CScintillaWnd, CWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()

BOOL CScintillaWnd::Create( LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext /*= NULL */ )
{
	return __super::Create("Scintilla",lpszWindowName,dwStyle,rect,pParentWnd,nID,pContext);
}

BOOL CScintillaWnd::CreateEx( DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam /*= NULL */ )
{
	return __super::CreateEx(dwExStyle,"Scintilla",lpszWindowName,dwStyle,x,y,nWidth,nHeight,hWndParent,nIDorHMenu,lpParam);
}

BOOL CScintillaWnd::CreateEx( DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam /*= NULL */ )
{
	return __super::CreateEx(dwExStyle,"Scintilla",lpszWindowName,dwStyle,rect,pParentWnd,nID,lpParam);
}



// CScintillaWnd 消息处理程序




int CScintillaWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	LRESULT	Ret = SendMessage(SCI_GETDIRECTFUNCTION);
	m_fnDirect = (::SciFnDirect) Ret;
	m_ptrDirect = (sptr_t)SendMessage(SCI_GETDIRECTPOINTER);

	// 	if (!m_fnDirect || !m_ptrDirect)
	// 	{
	// 		return FALSE;
	// 	}

	return 0;
}
