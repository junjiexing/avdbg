// StackView.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "StackView.h"
#include "AsmView.h"
#include "AppConfig.h"


// CStackView

IMPLEMENT_DYNAMIC(CStackView, CWnd)

CStackView::CStackView()
	:m_nLineHight(20),m_nFontWidth(20),m_dwStartAddr(0)
{

}

CStackView::~CStackView()
{
}


BEGIN_MESSAGE_MAP(CStackView, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CStackView::Create( const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return __super::Create(NULL,NULL,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL,rect,pParentWnd,nID);
}



// CStackView 消息处理程序




int CStackView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetPaintFont(app_cfg.asm_view_font);

	return 0;
}


void CStackView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CFont* pOldFont = dc.SelectObject(&m_Font);
	pOldFont->DeleteObject();
	
	RECT rc;
	GetClientRect(&rc);

	for (int i=0;i*m_nLineHight<=rc.bottom;++i)
	{
		RECT tmp = rc;
		tmp.top = i*m_nLineHight;
		tmp.bottom = tmp.top + m_nLineHight;
			
		DWORD data;
		DWORD dwLineAddr = m_dwStartAddr+i*16;
		SIZE_T nRead = 0;
		if (debug_kernel_ptr && debug_kernel_ptr->read_memory(dwLineAddr,&data,4,&nRead) && nRead == 4)
		{
			char line[20];
			sprintf(line,"%08X %08X",dwLineAddr,data);
			dc.ExtTextOut(0,i*m_nLineHight,ETO_OPAQUE,&tmp,line,17,NULL);
		}
		else
		{
			char line[20];
			sprintf(line,"%08X ???",dwLineAddr);
			dc.ExtTextOut(0,i*m_nLineHight,ETO_OPAQUE,&tmp,line,12,NULL);
		}
	}
	// 不为绘图消息调用 CWnd::OnPaint()
}
