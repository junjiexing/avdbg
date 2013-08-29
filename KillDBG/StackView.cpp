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
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
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
		DWORD dwLineAddr = m_dwStartAddr+i*4;
		SIZE_T nRead = 0;

		if (dwLineAddr>=m_dwSelStart && dwLineAddr <=m_dwSelEnd)
		{
			dc.SetBkColor(0x00FF0000);
		}

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

		dc.SetBkColor(0x00FFFFFF);
	}
	// 不为绘图消息调用 CWnd::OnPaint()
}


void CStackView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	m_bLBtnDwn = true;
	m_dwSelStart = m_dwSelEnd = m_dwStartAddr + point.y / m_nLineHight * 4;
	Invalidate(FALSE);

	CWnd::OnLButtonDown(nFlags, point);
}


void CStackView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bLBtnDwn = false;
	m_dwSelEnd = m_dwStartAddr + point.y / m_nLineHight * 4;
	Invalidate(FALSE);

	CWnd::OnLButtonUp(nFlags, point);
}


void CStackView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bLBtnDwn)
	{
		m_dwSelEnd = m_dwStartAddr + point.y / m_nLineHight * 4;
		Invalidate(FALSE);
	}

	CWnd::OnMouseMove(nFlags, point);
}


BOOL CStackView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_dwStartAddr += zDelta>0?-4:4;

	ScreenToClient(&pt);
	m_dwSelEnd = m_dwStartAddr + pt.y / m_nLineHight * 4;
	Invalidate(FALSE);

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}
