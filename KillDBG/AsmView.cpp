// AsmView.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "DebugKernel.h"
#include "AsmView.h"
#include "x86dis.h"
#include "kd_utils.h"

// CAsmView

IMPLEMENT_DYNAMIC(CAsmView, CWnd)

CAsmView::CAsmView()
	:m_AddrToShow(NULL),m_Eip(NULL),m_Decoder(X86_OPSIZE32,X86_ADDRSIZE32),
	m_dwSelAddrEnd(NULL),m_dwSelAddrStart(NULL)
{

}

CAsmView::~CAsmView()
{
}


BEGIN_MESSAGE_MAP(CAsmView, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEWHEEL()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CAsmView::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
// 	m_wndOutputWnd.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
// 		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL);
	m_AddrToShow = m_Eip = NULL;
	return __super::Create(NULL,lpszWindowName,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL,
		rect,pParentWnd,nID);
}



// CAsmView 消息处理程序




void CAsmView::OnPaint()
{
	//__super::OnPaint();
	CPaintDC dc(this); // device context for painting
	RECT	rcClient;
	RECT	rcLine = {0};
	GetClientRect(&rcClient);
	rcLine.right = rcClient.right;

	dc.FillRect(&rcClient,&CBrush(0xB2F7FF));

	if (debug_kernel_ptr == NULL)
	{
		return;
	}

	m_vecAddress.clear();

	byte buffer[rcClient.bottom/20*15+1];
	SIZE_T num_read;
	if (!debug_kernel_ptr->read_memory(m_AddrToShow,buffer,rcClient.bottom/20*15,&num_read))
	{
		int y = 0;
		for (int i=0;i<rcClient.bottom/20;++i)
		{
			char szInsn1[20];
			sprintf(szInsn1,"%08X  ???",m_AddrToShow+i);
			m_vecAddress.push_back(m_AddrToShow+i);
			rcLine.top = y;
			rcLine.bottom = y+20;
			dc.ExtTextOut(0,y,ETO_OPAQUE,&rcLine,szInsn1,strlen(szInsn1),NULL);
			y+=20;
		}

		return;
	}
	

	CPU_ADDR	curAddr = {0};
	curAddr.addr32.offset = (uint32)m_AddrToShow;
	int y = 0;
	for (unsigned int i=0;i<num_read;)
	{
		x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer+i,num_read-i,curAddr);
		if ((uint32)m_AddrToShow>curAddr.addr32.offset && (uint32)m_AddrToShow < curAddr.addr32.offset + insn->size)
		{
			break;
		}
		const char* pcsIns = m_Decoder.str(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD);

		char szInsn[100];
		sprintf(szInsn, "%08X  %s",curAddr.addr32.offset, pcsIns);
		m_vecAddress.push_back(curAddr.addr32.offset);
		if (y<rcClient.bottom && curAddr.addr32.offset >= (uint32)m_AddrToShow-5)
		{
			debug_kernel::breakpoint_t* bp;
			if (curAddr.addr32.offset == (uint32)m_Eip)
			{
				dc.SetBkColor(0x0000FF);
			}
			else if (curAddr.addr32.offset >= m_dwSelAddrStart && curAddr.addr32.offset <= m_dwSelAddrEnd)
			{
				dc.SetBkColor(0x00FFFF);
			}
			else if (bp = debug_kernel_ptr->find_breakpoint_by_address(curAddr.addr32.offset))
			{
				if (bp->user_enable)
				{
					dc.SetBkColor(0xFFFFFF);
				}
				else
				{
					dc.SetBkColor(0xFFFF00);
				}
			}
			else
			{
				dc.SetBkColor(0x00FF33);
			}

			rcLine.top = y;
			rcLine.bottom = y+20;
			dc.ExtTextOut(0,y,ETO_OPAQUE,&rcLine,szInsn,strlen(szInsn),NULL);
			y+=20;

		}
		i += insn->size;
		curAddr.addr32.offset += insn->size;

	}
}


BOOL CAsmView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	MessageBox(NULL);
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CAsmView::SetEIP( DWORD eip )
{

	kd_utils::scope_exit on_return([this](){UpdateScrollInfo();});

	m_Eip = eip;

	if (m_AddrToShow == eip)
	{
		return;
	}

	byte buffer[15*5];
	SIZE_T	num_read = 0;
	if (!debug_kernel_ptr || !debug_kernel_ptr->read_memory(m_AddrToShow,buffer,15*5,&num_read))
	{
		m_AddrToShow = m_Eip;
		return;
	}

	CPU_ADDR	curAddr = {0};
	curAddr.addr32.offset = (uint32)m_AddrToShow;
	bool found = false;
	int num_insn = 0;
	for (unsigned int i=0;i<num_read && num_insn <= 5; ++num_insn)
	{
		x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer+i,num_read-i,curAddr);
		i += insn->size;
		curAddr.addr32.offset += insn->size;
		if (curAddr.addr32.offset == (uint32)m_Eip)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		m_AddrToShow = m_Eip;
		return;
	}
	if (num_insn == 4)
	{
		x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer,num_read,curAddr);
		m_AddrToShow += insn->size;
	}
}



void CAsmView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch(nSBCode)
	{
	case SB_TOP:
		m_AddrToShow = NULL;
		break;

	case SB_BOTTOM:
		m_AddrToShow = 0x7FFEFFF;
		break;

	case SB_LINEUP:
		{
			DWORD PreCode = 0;
			PreviousCode(m_AddrToShow,&PreCode);
			m_AddrToShow = PreCode;
		}
		break;

	case SB_LINEDOWN:
		{
			byte buffer[16];
			SIZE_T	num_read = 0;
			if (!debug_kernel_ptr || !debug_kernel_ptr->read_memory(m_AddrToShow,buffer,15,&num_read))
			{
				m_AddrToShow += 1;
				return;
			}

			CPU_ADDR	curAddr = {0};
			curAddr.addr32.offset = (uint32)m_AddrToShow;
			x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer,num_read,curAddr);
			m_AddrToShow += insn->size;
		}
		break;

	case SB_PAGEDOWN:
		break;

	case SB_PAGEUP:
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
			SCROLLINFO scroll_info = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			GetScrollInfo(SB_VERT, &scroll_info);
			PreviousCode(scroll_info.nTrackPos,&m_AddrToShow);
		}
		break;
	}

	UpdateScrollInfo();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CAsmView::PreviousCode( DWORD TargetAddr,DWORD* PreInsn )
{
	if (TargetAddr == 0)
	{
		*PreInsn = 0;
		return;
	}

	if (TargetAddr < 40)
	{
		*PreInsn = TargetAddr - 1;
	}

	DWORD TmpAddr = TargetAddr - 40;

	byte buffer[41];
	SIZE_T nRead = 0;
	if (!debug_kernel_ptr->read_memory(TmpAddr,buffer,40,&nRead) || nRead != 40)
	{
		*PreInsn = TargetAddr - 1;
		return;
	}

	CPU_ADDR	curAddr = {0};
	curAddr.addr32.offset = (uint32)TmpAddr;
	for (unsigned int i=0;i<40;)
	{
		x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer+i,40-i,curAddr);
		i += insn->size;
		*PreInsn = curAddr.addr32.offset;
		curAddr.addr32.offset += insn->size;

		if (curAddr.addr32.offset >= TargetAddr)
		{
			return;
		}
	}
}


void CAsmView::OnLButtonDown(UINT nFlags, CPoint point)
{
	int index = point.y/20;
	if (index<m_vecAddress.size())
	{
		m_dwSelAddrEnd = m_dwSelAddrStart = m_vecAddress[index];
	}
	Invalidate();
	CWnd::OnLButtonDown(nFlags, point);
}

void CAsmView::UpdateScrollInfo()
{
	RECT rc = {0};
	GetClientRect(&rc);

	SCROLLINFO scroll_info = {sizeof(SCROLLINFO)};
	scroll_info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

	scroll_info.nMin = 0;
	scroll_info.nMax = 0x7FFEFFFF;
	scroll_info.nPos = m_AddrToShow;
	scroll_info.nPage = rc.bottom / 20;

	SetScrollInfo(SB_VERT,&scroll_info,TRUE);

	Invalidate(FALSE);
}
