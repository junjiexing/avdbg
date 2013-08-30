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
	ON_WM_CREATE()
END_MESSAGE_MAP()


BOOL COutputWnd::Create( const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return __super::Create(NULL,NULL,rect,pParentWnd,nID);
}


int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScintillaWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SciFnDirect(SCI_USEPOPUP,FALSE,NULL);	// 去掉右键菜单
	SciFnDirect(SCI_SETMARGINWIDTHN,1,0);	//去掉Margin
	SciFnDirect(SCI_SETCARETLINEVISIBLE, TRUE);  // 高亮当前行
	SciFnDirect(SCI_SETREADONLY,TRUE);	// 不允许用户输入
	return 0;
}
