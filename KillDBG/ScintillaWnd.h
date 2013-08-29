#pragma once
#include "Scintilla.h"


// CScintillaWnd

class CScintillaWnd : public CWnd
{
	DECLARE_DYNAMIC(CScintillaWnd)

public:
	CScintillaWnd();
	virtual ~CScintillaWnd();


	virtual BOOL Create( LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL );

	virtual BOOL CreateEx( DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam = NULL );

	virtual BOOL CreateEx( DWORD dwExStyle, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL );

	sptr_t SciFnDirect(unsigned int iMessage, uptr_t wParam=0, sptr_t lParam=0)
	{
		return m_fnDirect(m_ptrDirect,iMessage,wParam,lParam);
	}

protected:
	DECLARE_MESSAGE_MAP()

	::SciFnDirect m_fnDirect;
	sptr_t m_ptrDirect;

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


