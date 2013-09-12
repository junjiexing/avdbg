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
#include "DebugUtils.h"
#include "FollowAddressDlg.h"

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;
// CMemoryView

IMPLEMENT_DYNAMIC(CMemoryWnd, CWnd)

CMemoryWnd::CMemoryWnd()
	:m_nAddrWidth(0),m_nHexWidth(0),m_nAsciiWidth(0),
	m_dwSelEnd(0),m_dwSelStart(0),m_bLBtnDwn(false),
	m_nLineHight(20),m_nFontWidth(20),m_dwStartAddr(0)
{

}

CMemoryWnd::~CMemoryWnd()
{
}


BEGIN_MESSAGE_MAP(CMemoryWnd, CWnd)
	ON_WM_PAINT()
	ON_NOTIFY(HDN_ENDTRACKA, 123, &CMemoryWnd::OnHdnEndtrack)
	ON_NOTIFY(HDN_ENDTRACKW, 123, &CMemoryWnd::OnHdnEndtrack)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_FOLLOWADDR, &CMemoryWnd::OnFollowAddr)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()



// CMemoryView 消息处理程序

void CMemoryWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	RECT rcClient;
	GetClientRect(&rcClient);

	std::vector<unsigned char> vecBuffer(rcClient.bottom/m_nLineHight*16+1);
	std::vector<unsigned char> vecCpy(rcClient.bottom/m_nLineHight*16+1);
	unsigned char* data = vecBuffer.data();
	unsigned char* cpy = vecCpy.data();
	bool bReadSuc = debug_kernel_ptr?debug_kernel_ptr->read_memory(m_dwStartAddr,data,rcClient.bottom/m_nLineHight*16):false;

	CDC dcMem;
	dcMem.CreateCompatibleDC(&dc);
	CBitmap bmpMem;
	bmpMem.CreateCompatibleBitmap(&dc,rcClient.right,rcClient.bottom);
	CBitmap* pOldBmp = dcMem.SelectObject(&bmpMem);
	CFont* pOldFont = dcMem.SelectObject(&m_Font);
	dcMem.SetBkMode(TRANSPARENT);
	CBrush brush(0xB2F7FF);
	dcMem.FillRect(&rcClient,&brush);

	debug_utils::scope_exit on_exit([this,&dc,&rcClient,&dcMem,&pOldBmp,&bmpMem,pOldFont]()
	{
		dc.BitBlt(0,0,rcClient.right,rcClient.bottom,&dcMem,0,0,SRCCOPY);
		dcMem.SelectObject(pOldBmp);
		dcMem.SelectObject(pOldFont);
		bmpMem.DeleteObject();

	});

	DWORD dwSelStart = std::min(m_dwSelStart,m_dwSelEnd);
	DWORD dwSelEnd = std::max(m_dwSelStart,m_dwSelEnd);

	for (int i=0;i<rcClient.bottom/m_nLineHight;++i)
	{
		int y = i * m_nLineHight + 20;

		RECT  rcLine = rcClient;
		rcLine.top = y;
		rcLine.bottom = y + m_nLineHight;
		dcMem.ExtTextOut(0,0,ETO_OPAQUE,&rcLine,NULL,0,NULL);
		// 绘制地址
		char buffer[20];
		sprintf(buffer,"%08X",m_dwStartAddr + i*16);
		RECT rc = {0};
		rc.top = y;
		rc.bottom = rc.top + m_nLineHight;
		rc.right = m_nAddrWidth;
		dcMem.ExtTextOut(0,y,ETO_CLIPPED|ETO_OPAQUE,&rc,buffer,8,NULL);

		// 绘制hex数据
		int width = 0;
		for (int j=0;j<16;++j)
		{
			DWORD pos = i*16+j;

			rc.left = m_nAddrWidth + width;
			rc.right = m_nAddrWidth + width + m_nFontWidth*3;
			rc.top = y;
			rc.bottom = rc.top + m_nLineHight;
			if (pos+m_dwStartAddr>=dwSelStart && pos+m_dwStartAddr<=dwSelEnd)
			{
				dcMem.SetBkColor(0x00FF0000);
			}

			if (bReadSuc)
			{
				sprintf(buffer,"%02X ",data[pos]);
			}
			else
			{
				strcpy(buffer,"?? ");
			}

			dcMem.ExtTextOut(m_nAddrWidth + width,y,ETO_CLIPPED|ETO_OPAQUE,&rc,buffer,3,NULL);
			dcMem.SetBkColor(0x00FFFFFF);
			width += m_nFontWidth*3;
		}

		// 绘制字符数据
		rc.left = m_nAddrWidth+m_nHexWidth;
		rc.right = m_nAddrWidth+m_nHexWidth+m_nFontWidth*16;
		rc.top = y;
		rc.bottom = y + m_nLineHight;
		dcMem.ExtTextOut(0,0,ETO_OPAQUE,&rc,NULL,0,NULL);

		int num = 16;
		if (i*16+16 > vecBuffer.size())
		{
			num = vecBuffer.size() - i*16;
		}

		dcMem.SetBkColor(0x00FF0000);

		DWORD dwLineStart = m_dwStartAddr+i*16;
		DWORD dwLineEnd = dwLineStart+15;
		if (dwLineStart>=dwSelStart && dwLineEnd<=dwSelEnd)
		{
			dcMem.ExtTextOut(0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
		}
		else if (dwLineStart>dwSelStart && dwLineStart<dwSelEnd && dwLineEnd>dwSelEnd)
		{
			RECT tmp = rc;
			tmp.right = m_nAddrWidth+m_nHexWidth+m_nFontWidth*(dwSelEnd-dwLineStart);
			dcMem.ExtTextOut(0,0,ETO_OPAQUE,&tmp,NULL,0,NULL);
		}
		else if (dwLineStart<dwSelStart && dwLineEnd>dwSelStart && dwLineEnd<dwSelEnd)
		{
			RECT tmp = rc;
			tmp.left = m_nAddrWidth+m_nHexWidth+m_nFontWidth*(dwSelStart-dwLineStart);
			tmp.right = tmp.left+m_nFontWidth*(num-(dwSelStart-dwLineStart));
			dcMem.ExtTextOut(0,0,ETO_OPAQUE,&tmp,NULL,0,NULL);
		}
		else if (dwLineStart<=dwSelStart && dwLineEnd>=dwSelEnd)
		{
			RECT tmp = rc;
			tmp.left = m_nAddrWidth+m_nHexWidth+m_nFontWidth*(dwSelStart-dwLineStart);
			tmp.right = tmp.left+m_nFontWidth*(dwSelEnd-dwSelStart+1);
			dcMem.ExtTextOut(0,0,ETO_OPAQUE,&tmp,NULL,0,NULL);
		}

		dcMem.SetBkColor(0x00FFFFFF);

		if (bReadSuc)
		{
			memcpy(buffer,(char*)data + (i*16),num);
		}
		else
		{
			strcpy(buffer,"????????????????");
		}
		dcMem.ExtTextOut(m_nAddrWidth+m_nHexWidth,y,ETO_CLIPPED,&rc,buffer,num,NULL);
	}
}

BOOL CMemoryWnd::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	return __super::Create(NULL,lpszWindowName,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL,rect,pParentWnd,nID);
}

void CMemoryWnd::OnHdnEndtrack( NMHDR *pNMHDR, LRESULT *pResult )
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
		m_nHexWidth = pitem->cxy;
		break;
	case 2:
		m_nAsciiWidth = pitem->cxy;
		break;
	}

	Invalidate(FALSE);
}

void CMemoryWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	RECT rc = {0};
	GetClientRect(&rc);
	rc.top = 20;
	rc.left = m_nAddrWidth;
	rc.right = m_nAddrWidth+m_nFontWidth*3*16; 
	POINT pt = {point.x,point.y};
	if (PtInRect(&rc,pt))
	{
		m_bLBtnDwn = true;

		int line = (point.y-20) / m_nLineHight;
		int nChar = (point.x-m_nAddrWidth)/(m_nFontWidth*3);

		m_dwSelEnd = m_dwSelStart = m_dwStartAddr + line*16+nChar;

		Invalidate();
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CMemoryWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	RECT rc = {0};
	GetClientRect(&rc);
	rc.top = 20;
	rc.left = m_nAddrWidth;
	rc.right = m_nAddrWidth+m_nFontWidth*3*16; 
	POINT pt = {point.x,point.y};
	if (PtInRect(&rc,pt))
	{
		m_bLBtnDwn = false;

		int line = (point.y-20) / m_nLineHight;
		int nChar = (point.x-m_nAddrWidth)/(m_nFontWidth*3);

		m_dwSelEnd = m_dwStartAddr + line*16+nChar;

		Invalidate();
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CMemoryWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	RECT rc = {0};
	GetClientRect(&rc);
	rc.top = 20;
	rc.left = m_nAddrWidth;
	rc.right = m_nAddrWidth+m_nFontWidth*3*16; 
	POINT pt = {point.x,point.y};
	if (PtInRect(&rc,pt) && m_bLBtnDwn)
	{
		int line = (point.y-20) / m_nLineHight;
		int nChar = (point.x-m_nAddrWidth)/(m_nFontWidth*3);

		m_dwSelEnd = m_dwStartAddr + line*16+nChar;

		Invalidate();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CMemoryWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RECT rc = {0};
	GetClientRect(&rc);
	m_Header.MoveWindow(0,0,rc.right,20);
}

int CMemoryWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	RECT rc;
	GetClientRect(&rc);
	m_Header.Create(NULL,rc,this,123);
	HDITEM	item = {0};
	item.mask = HDI_WIDTH | HDI_TEXT;
	m_nAddrWidth = item.cxy = 120;
	item.pszText = "地址";
	m_Header.InsertItem(0,&item);
	m_nHexWidth = item.cxy = 240;
	item.pszText = "HEX";
	m_Header.InsertItem(1,&item);
	m_nAsciiWidth = item.cxy = 120;
	item.pszText = "ASCII";
	m_Header.InsertItem(2,&item);
	m_Header.ShowWindow(SW_SHOW);

	SetPaintFont(app_cfg.font_cfg.asm_view_font);

	m_Menu.CreatePopupMenu();
	m_Menu.AppendMenu(MF_ENABLED,ID_FOLLOWADDR,"转到地址");

	return 0;
}

void CMemoryWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	POINT pt = {point.x,point.y};
	ClientToScreen(&pt);
	m_Menu.TrackPopupMenu(NULL,pt.x,pt.y,this);

	CWnd::OnRButtonDown(nFlags, point);
}

void CMemoryWnd::OnFollowAddr()
{
	CFollowAddressDlg dlg;
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	
	SetAddrToView(dlg.m_dwAddr);
}


void CMemoryWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	RECT rc;
	GetClientRect(&rc);

	switch(nSBCode)
	{
	case SB_TOP:
		m_dwStartAddr = NULL;
		break;

	case SB_BOTTOM:
		m_dwStartAddr = 0x7FFEFFF;
		break;

	case SB_LINEUP:
		{
			if (m_dwStartAddr >= 16)
			{
				m_dwStartAddr -= 16;
			} 
		}
		break;

	case SB_LINEDOWN:
		{
			if (m_dwStartAddr <= 0x7FFEFFFF-16)
			{
				m_dwStartAddr += 16;
			}
		}
		break;

	case SB_PAGEDOWN:
		{
			int nLines = rc.bottom / m_nLineHight;
			if (m_dwStartAddr <= 0x7FFEFFFF - nLines * 16)
			{
				m_dwStartAddr += nLines * 16;
			}
		}
		break;

	case SB_PAGEUP:
		{
			int nLines = rc.bottom / m_nLineHight;
			if (m_dwStartAddr >= nLines * 16)
			{
				m_dwStartAddr -= nLines * 16;
			}
		}
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
			SCROLLINFO scroll_info = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			GetScrollInfo(SB_VERT, &scroll_info);
			m_dwStartAddr = scroll_info.nTrackPos;
		}
		break;
	}

	UpdateScrollInfo();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CMemoryWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	RECT rc;
	GetClientRect(&rc);

	if (zDelta>0 && m_dwStartAddr>=16)
	{
		m_dwStartAddr -= 16;
	}
	else if (zDelta<0 && m_dwStartAddr<0x7FFEFFFF-16)
	{
		m_dwStartAddr += 16;
	}

	UpdateScrollInfo();

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CMemoryWnd::UpdateScrollInfo()
{
	RECT rc = {0};
	GetClientRect(&rc);

	SCROLLINFO scroll_info = {sizeof(SCROLLINFO)};
	scroll_info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

	scroll_info.nMin = 0;
	scroll_info.nMax = 0x7FFEFFFF;
	scroll_info.nPos = m_dwStartAddr;
	scroll_info.nPage = rc.bottom / m_nLineHight;

	SetScrollInfo(SB_VERT,&scroll_info,TRUE);

	Invalidate(FALSE);
}
