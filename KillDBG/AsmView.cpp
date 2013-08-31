// AsmView.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "DebugKernel.h"
#include "AsmView.h"
#include "x86dis.h"
#include "DebugUtils.h"
#include "AssemblyDlg.h"
#include "AppConfig.h"

// CAsmView

IMPLEMENT_DYNAMIC(CAsmView, CWnd)

CAsmView::CAsmView()
	:m_AddrToShow(NULL),m_Eip(NULL),m_Decoder(X86_OPSIZE32,X86_ADDRSIZE32),
	m_dwSelAddrEnd(NULL),m_dwSelAddrStart(NULL),m_nLineHight(20),m_nFontWidth(20),
	m_bLButtonDown(FALSE)
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
	ON_WM_CHAR()
	ON_WM_CREATE()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
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

	CDC dcMem;
	dcMem.CreateCompatibleDC(&dc);
	CBitmap bmpMem;
	bmpMem.CreateCompatibleBitmap(&dc,rcClient.right,rcClient.bottom);
	CBitmap* pOldBmp = dcMem.SelectObject(&bmpMem);
	CFont* pOldFont = dcMem.SelectObject(&m_Font);


	debug_utils::scope_exit on_exit([this,&dc,&rcClient,&dcMem,&pOldBmp,&bmpMem,pOldFont]()
	{
		dc.BitBlt(0,0,rcClient.right,rcClient.bottom,&dcMem,0,0,SRCCOPY);
		dcMem.SelectObject(pOldBmp);
		dcMem.SelectObject(pOldFont);
		bmpMem.DeleteObject();

	});


	if (debug_kernel_ptr == NULL)
	{
		dcMem.FillRect(&rcClient,&CBrush(0x00FFFFFF));
		return;
	}

	m_vecAddress.clear();

	byte buffer[rcClient.bottom/m_nLineHight*15+1];
	SIZE_T num_read;
	if (!debug_kernel_ptr->read_memory(m_AddrToShow,buffer,rcClient.bottom/m_nLineHight*15,&num_read))
	{
		int y = 0;
		for (int i=0;i<rcClient.bottom/m_nLineHight;++i)
		{
			char szUnknownInsn[20];
			sprintf(szUnknownInsn,"%08X  ???",m_AddrToShow+i);
			m_vecAddress.push_back(m_AddrToShow+i);
			rcLine.top = y;
			rcLine.bottom = y+m_nLineHight;
			dcMem.ExtTextOut(0,y,ETO_OPAQUE,&rcLine,szUnknownInsn,strlen(szUnknownInsn),NULL);
			y+=m_nLineHight;
		}

		return;
	}
	
	dcMem.SetBkMode(TRANSPARENT);
	CPU_ADDR	curAddr = {0};
	curAddr.addr32.offset = (uint32)m_AddrToShow;
	for (unsigned int i=0,j=0;i<num_read;++j)
	{
		x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer+i,num_read-i,curAddr);
		if ((uint32)m_AddrToShow>curAddr.addr32.offset && (uint32)m_AddrToShow < curAddr.addr32.offset + insn->size)
		{
			break;
		}

		//dcMem.ExtTextOut()
		// 绘制当前行的背景色
		RECT rcLine;	// 当前行的矩形范围
		rcLine.top = j*m_nLineHight;
		rcLine.bottom = rcLine.top+m_nLineHight;
		rcLine.left = 0;
		rcLine.right = rcClient.right;

		debug_kernel::breakpoint_t* bp;
		if (curAddr.addr32.offset == (uint32)m_Eip)	// EIP在这一行
		{
			dcMem.SetBkColor(0x00B26600);
		}
		else if (bp = debug_kernel_ptr->find_breakpoint_by_address(curAddr.addr32.offset)) // 这一行有断点
		{
			if (bp->user_enable) // 是否启用了
			{
				dcMem.SetBkColor(0x000000FF);
			}
			else
			{
				dcMem.SetBkColor(0x00990066);
			}
		}
		else if (curAddr.addr32.offset >= m_dwSelAddrStart && curAddr.addr32.offset <= m_dwSelAddrEnd) // 这一行被选中了
		{
			dcMem.SetBkColor(0x0000FFFF);
		}
		else  // 普通行
		{
			dcMem.SetBkColor(0x00FFFFFF);
		}

		dcMem.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rcLine,NULL,NULL,NULL);	// 用指定的背景色给当前航上色

		x86dis_str str = {0};
		m_Decoder.str_insn(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD,str);
		//const char* pcsIns = m_Decoder.str(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD);
		char pcsIns[1024] = {0};
		//sprintf(pcsIns,"%s %s,%s,%s,%s,%s,%s",str.prefix,str.opcode,str.operand[0],str.operand[1],str.operand[2],str.operand[3],str.operand[4]);
		if (str.prefix[0])
		{
			strcat(pcsIns,str.prefix);
		}
		if (str.opcode[0])
		{
			strcat(pcsIns," ");
			strcat(pcsIns,str.opcode);
		}
		if (str.operand[0][0])
		{
			strcat(pcsIns," ");
			strcat(pcsIns,str.operand[0]);
		}
		if (str.operand[1][0])
		{
			strcat(pcsIns,",");
			strcat(pcsIns,str.operand[1]);
		}
		if (str.operand[2][0])
		{
			strcat(pcsIns,",");
			strcat(pcsIns,str.operand[2]);
		}
		if (str.operand[3][0])
		{
			strcat(pcsIns,",");
			strcat(pcsIns,str.operand[3]);
		}
		if (str.operand[4][0])
		{
			strcat(pcsIns,",");
			strcat(pcsIns,str.operand[40]);
		}


		char szInsn[100];
		sprintf(szInsn, "%08X  %s",curAddr.addr32.offset, pcsIns);
		m_vecAddress.push_back(curAddr.addr32.offset);
		dcMem.ExtTextOut(0,j*m_nLineHight,NULL,NULL,szInsn,strlen(szInsn),NULL);

		i += insn->size;
		curAddr.addr32.offset += insn->size;
	}
}

BOOL CAsmView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	do 
	{
		if (!debug_kernel_ptr)
		{
			break;;
		}

		if (m_bLButtonDown)
		{
			ScreenToClient(&pt);
			int index = pt.y/m_nLineHight;
			if (index<m_vecAddress.size())
			{
				m_dwSelAddrEnd = m_vecAddress[index];
			}
			Invalidate(FALSE);
		}



		if (zDelta<0)
		{
			byte buffer[15*5];
			SIZE_T	num_read = 0;
			if (!debug_kernel_ptr->read_memory(m_AddrToShow,buffer,15*5,&num_read))
			{
				m_AddrToShow += 1;
				break;;
			}

			CPU_ADDR	curAddr = {0};
			curAddr.addr32.offset = (uint32)m_AddrToShow;
			for (int i=0,j=0;i<3 && j<num_read;++i)
			{
				x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer+j,num_read-j,curAddr);
				j+=insn->size;
				curAddr.addr32.offset += insn->size;
			}
			m_AddrToShow = curAddr.addr32.offset;
			break;
		}

		DWORD dwTmpAddr = m_AddrToShow;
		for (int i=0;i<3;++i)
		{
			PreviousCode(dwTmpAddr,&dwTmpAddr);
		}
		m_AddrToShow = dwTmpAddr;
	} while (0);
	
	Invalidate(FALSE);
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CAsmView::SetEIP( DWORD eip )
{

	debug_utils::scope_exit on_return([this](){UpdateScrollInfo();});

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

void CAsmView::PreviousCode( DWORD dwTargetAddr,DWORD* pdwPreInsn )
{
	if (dwTargetAddr == 0)
	{
		*pdwPreInsn = 0;
		return;
	}

	if (dwTargetAddr < 40)
	{
		*pdwPreInsn = dwTargetAddr - 1;
	}

	DWORD TmpAddr = dwTargetAddr - 40;

	byte buffer[41];
	SIZE_T nRead = 0;
	if (!debug_kernel_ptr->read_memory(TmpAddr,buffer,40,&nRead) || nRead != 40)
	{
		*pdwPreInsn = dwTargetAddr - 1;
		return;
	}

	CPU_ADDR	curAddr = {0};
	curAddr.addr32.offset = (uint32)TmpAddr;
	for (unsigned int i=0;i<40;)
	{
		x86dis_insn* insn = (x86dis_insn*)m_Decoder.decode(buffer+i,40-i,curAddr);
		i += insn->size;
		*pdwPreInsn = curAddr.addr32.offset;
		curAddr.addr32.offset += insn->size;

		if (curAddr.addr32.offset >= dwTargetAddr)
		{
			return;
		}
	}
}

void CAsmView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	m_bLButtonDown = TRUE;

	int index = point.y/m_nLineHight;
	if (index<m_vecAddress.size())
	{
		m_dwSelAddrEnd = m_dwSelAddrStart = m_vecAddress[index];
	}
	Invalidate(FALSE);
	CWnd::OnLButtonDown(nFlags, point);
}

void CAsmView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bLButtonDown = FALSE;

	int index = point.y/m_nLineHight;
	if (index<m_vecAddress.size())
	{
		m_dwSelAddrEnd = m_vecAddress[index];
	}
	Invalidate(FALSE);
	CWnd::OnLButtonUp(nFlags, point);
}


void CAsmView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bLButtonDown)
	{
		int index = point.y/m_nLineHight;
		if (index<m_vecAddress.size())
		{
			m_dwSelAddrEnd = m_vecAddress[index];
		}
		Invalidate(FALSE);
	}

	CWnd::OnMouseMove(nFlags, point);
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
	scroll_info.nPage = rc.bottom / m_nLineHight;

	SetScrollInfo(SB_VERT,&scroll_info,TRUE);

	Invalidate(FALSE);
}

void CAsmView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	debug_utils::scope_exit on_exit([this,nChar,nRepCnt,nFlags](){CWnd::OnChar(nChar, nRepCnt, nFlags);});
	if (nChar != ' ' || m_AddrToShow == 0)
	{
		return;
	}

	CAssemblyDlg dlg;
	dlg.m_dwAddress = m_dwSelAddrStart;
	if (!debug_kernel_ptr->read_memory(m_dwSelAddrStart,dlg.m_OrgOpcode,15))
	{
		return;
	}

	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	if (!debug_kernel_ptr->write_memory(m_dwSelAddrStart,dlg.m_NewOpcode,dlg.m_NewOpcSize))
	{
		MessageBox("向目标进程写入代码失败","错误",MB_OK|MB_ICONERROR);
		return;
	}
	Invalidate(FALSE);
}

int CAsmView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetPaintFont(app_cfg.font_cfg.asm_view_font);

	return 0;
}

