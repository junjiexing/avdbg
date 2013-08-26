// MemoryView.cpp : 实现文件
//

#include "stdafx.h"
#include "MemoryView.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <algorithm>
#include "DebugKernel.h"
#include "AppConfig.h"

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;
// CMemoryView

IMPLEMENT_DYNAMIC(CMemoryView, CWnd)

CMemoryView::CMemoryView()
	:m_AddrWidth(0),m_HexWidth(0),m_AsciiWidth(0),
	m_dwSelEnd(0),m_dwSelStart(0),m_bLBtnDwn(false)
{

}

CMemoryView::~CMemoryView()
{
}


BEGIN_MESSAGE_MAP(CMemoryView, CWnd)
	ON_WM_PAINT()
	ON_NOTIFY(HDN_ENDTRACKA, 123, &CMemoryView::OnHdnEndtrack)
	ON_NOTIFY(HDN_ENDTRACKW, 123, &CMemoryView::OnHdnEndtrack)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_CREATE()
END_MESSAGE_MAP()



// CMemoryView 消息处理程序

void CMemoryView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	dc.SetBkMode(TRANSPARENT);
	
	RECT rcClient;
	GetClientRect(&rcClient);
	dc.FillRect(&rcClient,&CBrush(0xB2F7FF));

	m_vecBuffer.resize(rcClient.bottom/20*16+1);
	unsigned char* data = m_vecBuffer.data();
	if (!debug_kernel_ptr->read_memory(m_dwStartAddr,data,rcClient.bottom/20*16))
	{
		return;
	}

	int nSelStart = std::min(m_dwSelStart,m_dwSelEnd);
	int nSelEnd = std::max(m_dwSelStart,m_dwSelEnd);

	CFont* pOldFont = dc.SelectObject(&m_Font);

	for (int i=0;i<rcClient.bottom;++i)
	{
		int y = i * 20 + 20;

		RECT  rcLine = rcClient;
		rcLine.top = y;
		rcLine.bottom = y + 20;
		dc.ExtTextOut(0,0,ETO_OPAQUE,&rcLine,NULL,0,NULL);
		// 绘制地址
		char buffer[10];
		sprintf(buffer,"%08X",m_dwStartAddr + i*16);
		RECT rc = {0};
		rc.top = y;
		rc.bottom = rc.top + 20;
		rc.right = m_AddrWidth;
		dc.ExtTextOut(0,y,ETO_CLIPPED|ETO_OPAQUE,&rc,buffer,8,NULL);

		// 绘制hex数据
		int width = 0;
		for (int j=0;j<16;++j)
		{
			DWORD pos = i*16+j;
			sprintf(buffer,"%02X ",data[pos]);

			rc.left = m_AddrWidth + width;
			rc.right = m_AddrWidth + width + 25;
			rc.top = y;
			rc.bottom = rc.top + 20;
			if (pos>=nSelStart && pos<=nSelEnd)
			{
				dc.SetBkColor(0x00FF0000);
			}
			dc.ExtTextOut(m_AddrWidth + width,y,ETO_CLIPPED|ETO_OPAQUE,&rc,buffer,3,NULL);
			dc.SetBkColor(0x00FFFFFF);
			width += dc.GetTextExtent(buffer,3).cx;
		}

		// 绘制字符数据
		rc.left = m_AddrWidth+m_HexWidth;
		rc.right = m_AddrWidth+m_HexWidth+m_AsciiWidth;
		rc.top = y;
		rc.bottom = y + 20;
		dc.ExtTextOut(0,0,ETO_OPAQUE,&rc,NULL,0,NULL);

		int num = 16;
		if (i*16+16 > m_vecBuffer.size())
		{
			num = m_vecBuffer.size() - i*16;
		}

		dc.SetBkColor(0x00FF0000);
		if (i*16>=nSelStart && i*16<=nSelEnd && i*16+num>=nSelStart && i*16+num<=nSelEnd)
		{
			dc.ExtTextOut(0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
		}
		else if (nSelStart>=i*16 && nSelStart<=i*16+num && nSelEnd>=i*16 && nSelEnd<=i*16+num)
		{
			RECT tmp = rc;
			tmp.left = m_AddrWidth+m_HexWidth+dc.GetTextExtent((char*)data+(i*16),nSelStart-i*16).cx;
			tmp.right = tmp.left+dc.GetTextExtent((char*)data+nSelStart,nSelEnd-nSelStart+1).cx;
			dc.ExtTextOut(0,0,ETO_OPAQUE,&tmp,NULL,0,NULL);
		}
		else if (i*16<=nSelStart && i*16+num>=nSelStart && i*16+num<=nSelEnd)
		{
			RECT tmp = rc;
			tmp.left = m_AddrWidth+m_HexWidth+dc.GetTextExtent((char*)data+(i*16),nSelStart-i*16).cx;
			dc.ExtTextOut(0,0,ETO_OPAQUE,&tmp,NULL,0,NULL);
		}
		else if (i*16>=nSelStart && i*16<=nSelEnd && i*16+num>=nSelEnd)
		{
			RECT tmp = rc;
			tmp.right = m_AddrWidth+m_HexWidth+dc.GetTextExtent((char*)data+(i*16),nSelEnd-i*16).cx;
			dc.ExtTextOut(0,0,ETO_OPAQUE,&tmp,NULL,0,NULL);
		}

		dc.SetBkColor(0x00FFFFFF);
		dc.ExtTextOut(m_AddrWidth+m_HexWidth,y,ETO_CLIPPED,&rc,(char*)data + (i*16),num,NULL);
	}
}

BOOL CMemoryView::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	__super::Create(NULL,lpszWindowName,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL,rect,pParentWnd,nID);

	RECT rc = {0};
	rc.bottom = 20;
	rc.right = rect.right;

	return TRUE;
}

void CMemoryView::OnHdnEndtrack( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;

	HDITEMA *pitem = phdr->pitem;
	switch (phdr->iItem)
	{
	case 0:
		m_AddrWidth = pitem->cxy;
		break;
	case 1:
		m_HexWidth = pitem->cxy;
		break;
	case 2:
		m_AsciiWidth = pitem->cxy;
		break;
	}

	Invalidate(FALSE);
}

void CMemoryView::OnLButtonDown(UINT nFlags, CPoint point)
{
	RECT rc = {0};
	GetClientRect(&rc);
	rc.top = 20;
	rc.left = m_AddrWidth;
	rc.right = m_AddrWidth+m_HexWidth; 
	POINT pt = {point.x,point.y};
	if (PtInRect(&rc,pt))
	{
		m_bLBtnDwn = true;

		unsigned char* data = m_vecBuffer.data();
		int line = (point.y-20) / 20;
		char buffer[5];
		int width = m_AddrWidth;
		CDC* dc = GetDC();
		for (int j=0;j<16;++j)
		{
			sprintf(buffer,"%02X ",data[line*16+j]);

			width += dc->GetTextExtent(buffer,3).cx;
			if (width>=point.x)
			{
				m_dwSelEnd = m_dwSelStart = line*16+j;
				break;
			}
		}
		ReleaseDC(dc);

		Invalidate();
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CMemoryView::OnLButtonUp(UINT nFlags, CPoint point)
{
	RECT rc = {0};
	GetClientRect(&rc);
	rc.top = 20;
	rc.left = m_AddrWidth;
	rc.right = m_AddrWidth+m_HexWidth; 
	POINT pt = {point.x,point.y};
	if (PtInRect(&rc,pt))
	{
		m_bLBtnDwn = false;

		unsigned char* data = m_vecBuffer.data();
		int line = (point.y-20) / 20;
		char buffer[5];
		int width = m_AddrWidth;
		CDC* dc = GetDC();
		for (int j=0;j<16;++j)
		{
			sprintf(buffer,"%02X ",data[line*16+j]);

			width += dc->GetTextExtent(buffer,3).cx;
			if (width>=point.x)
			{
				m_dwSelEnd = line*16+j;
				break;
			}
		}
		ReleaseDC(dc);

		Invalidate();
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CMemoryView::OnMouseMove(UINT nFlags, CPoint point)
{
	RECT rc = {0};
	GetClientRect(&rc);
	rc.top = 20;
	rc.left = m_AddrWidth;
	rc.right = m_AddrWidth+m_HexWidth; 
	POINT pt = {point.x,point.y};
	if (PtInRect(&rc,pt) && m_bLBtnDwn)
	{
		unsigned char* data = m_vecBuffer.data();
		int line = (point.y-20) / 20;
		char buffer[5];
		int width = m_AddrWidth;
		CDC* dc = GetDC();
		for (int j=0;j<16;++j)
		{
			sprintf(buffer,"%02X ",data[line*16+j]);

			width += dc->GetTextExtent(buffer,3).cx;
			if (width>=point.x)
			{
				m_dwSelEnd = line*16+j;
				break;
			}
		}
		ReleaseDC(dc);

		Invalidate();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CMemoryView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RECT rc = {0};
	GetClientRect(&rc);
	m_Header.MoveWindow(0,0,rc.right,20);
}

int CMemoryView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	RECT rc;
	GetClientRect(&rc);
	m_Header.Create(NULL,rc,this,123);
	HDITEM	item = {0};
	item.mask = HDI_WIDTH | HDI_TEXT;
	m_AddrWidth = item.cxy = 120;
	item.pszText = "地址";
	m_Header.InsertItem(0,&item);
	m_HexWidth = item.cxy = 240;
	item.pszText = "HEX";
	m_Header.InsertItem(1,&item);
	m_AsciiWidth = item.cxy = 120;
	item.pszText = "ASCII";
	m_Header.InsertItem(2,&item);
	m_Header.ShowWindow(SW_SHOW);

	SetPaintFont(app_cfg.asm_view_font);

	return 0;
}
