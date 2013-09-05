// CallStackWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "DebugKernel.h"
#include "CallStackWnd.h"
#include "AppConfig.h"

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;

// CCallStackWnd

IMPLEMENT_DYNAMIC(CCallStackWnd, CWnd)

CCallStackWnd::CCallStackWnd()
{

}

CCallStackWnd::~CCallStackWnd()
{
}


BEGIN_MESSAGE_MAP(CCallStackWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(HDN_ENDTRACKA, 123, &CCallStackWnd::OnHdnEndtrack)
	ON_NOTIFY(HDN_ENDTRACKW, 123, &CCallStackWnd::OnHdnEndtrack)
END_MESSAGE_MAP()



// CCallStackWnd 消息处理程序




void CCallStackWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	RECT rcClient;
	GetClientRect(&rcClient);
	CBrush brush(0x00FFFFFF);
	dc.FillRect(&rcClient,&brush);
	if (!debug_kernel_ptr)
	{
		return;
	}

	CFont* pOldFont = dc.SelectObject(&m_Font);
	pOldFont->DeleteObject();

	CONTEXT context = debug_kernel_ptr->get_current_context();
	STACKFRAME64 frame = {0};
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrPC.Offset = context.Eip;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Esp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Ebp;

	int y = 20;
	while (debug_kernel_ptr->stack_walk(frame,context))
	{
		char buffer[100];
		std::string symbol;
		sprintf(buffer,"%08X",(DWORD)frame.AddrPC.Offset);
		if (debug_kernel_ptr->symbol_from_addr((DWORD)frame.AddrPC.Offset,symbol,true))
		{
			strcat(buffer,"(");
			strcat(buffer,symbol.c_str());
			strcat(buffer,")");
		}

		RECT rc = {0};

		rc.top = y;
		rc.bottom = y+20;
		rc.right = m_nAddrWidth;
		dc.ExtTextOut(0,y,ETO_CLIPPED,&rc,buffer,strlen(buffer),NULL);

		sprintf(buffer,"%08X",(DWORD)frame.AddrReturn.Offset);
		rc.left = rc.right;
		rc.right = rc.left + m_nRetAddrWidth;
		dc.ExtTextOut(rc.left,y,ETO_CLIPPED,&rc,buffer,8,NULL);

		sprintf(buffer,"%08X",(DWORD)frame.AddrFrame.Offset);
		rc.left = rc.right;
		rc.right = rc.left + m_nFrameWidth;
		dc.ExtTextOut(rc.left,y,ETO_CLIPPED,&rc,buffer,8,NULL);

		sprintf(buffer,"%08X",(DWORD)frame.AddrStack.Offset);
		rc.left = rc.right;
		rc.right = rc.left + m_nStackWidth;
		dc.ExtTextOut(rc.left,y,ETO_CLIPPED,&rc,buffer,8,NULL);

		y += 20;
	}

}

BOOL CCallStackWnd::Create( const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return __super::Create(NULL,NULL,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL,
		rect,pParentWnd,nID);
}


int CCallStackWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	RECT rc;
	GetClientRect(&rc);
	m_Header.Create(NULL,rc,this,123);
	HDITEM	item = {0};
	item.mask = HDI_WIDTH | HDI_TEXT;
	m_nAddrWidth = item.cxy = 100;
	item.pszText = "地址";
	m_Header.InsertItem(0,&item);
	m_nRetAddrWidth = item.cxy = 100;
	item.pszText = "返回地址";
	m_Header.InsertItem(1,&item);
	m_nFrameWidth = item.cxy = 100;
	item.pszText = "栈帧";
	m_Header.InsertItem(2,&item);
	m_nStackWidth = item.cxy = 100;
	item.pszText = "堆栈";
	m_Header.InsertItem(3,&item);
	m_Header.ShowWindow(SW_SHOW);

	SetPaintFont(app_cfg.font_cfg.stk_view_font);

	return 0;
}


void CCallStackWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RECT rc = {0};
	GetClientRect(&rc);
	m_Header.MoveWindow(0,0,rc.right,20);
}


void CCallStackWnd::OnHdnEndtrack( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;

	HDITEMA *pitem = phdr->pitem;
	switch (phdr->iItem)
	{
	case 0:
		m_nAddrWidth = pitem->cxy;
		break;
	case 1:
		m_nRetAddrWidth = pitem->cxy;
		break;
	case 2:
		m_nFrameWidth = pitem->cxy;
		break;
	case 3:
		m_nStackWidth = pitem->cxy;
	}

	Invalidate(FALSE);
}