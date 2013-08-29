#include "stdafx.h"
#include "OutputWnd.h"


IMPLEMENT_DYNAMIC(COutputWnd, CWnd);

COutputWnd::COutputWnd(void)
{
}


COutputWnd::~COutputWnd(void)
{
}

BEGIN_MESSAGE_MAP(COutputWnd, CWnd)
END_MESSAGE_MAP()


BOOL COutputWnd::Create( const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return __super::Create(NULL,NULL,rect,pParentWnd,nID);
}


BOOL COutputWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_CHAR)
	{
		return TRUE;
	}

	return CScintillaWnd::PreTranslateMessage(pMsg);
}

