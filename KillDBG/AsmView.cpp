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

IMPLEMENT_DYNAMIC(CAsmWnd, CWnd)

CAsmWnd::CAsmWnd()
	:m_AddrToShow(NULL),m_Eip(NULL),m_Decoder(X86_OPSIZE32,X86_ADDRSIZE32),
	m_dwSelAddrEnd(NULL),m_dwSelAddrStart(NULL),m_nLineHight(20),m_nFontWidth(20),
	m_bLButtonDown(FALSE),m_nMargenWidth(20)
{
	m_Decoder.set_addr_sym_func([](CPU_ADDR addr, int *symstrlen, X86_Optype type)->const char*
	{
		if (!debug_kernel_ptr)
		{
			return NULL;
		}

		static std::string symbol;
		if (debug_kernel_ptr->symbol_from_addr(addr.addr32.offset,symbol))
		{
			*symstrlen = symbol.size();
			return symbol.c_str();
		}

		if (type == X86_OPTYPE_MEM)
		{
			SIZE_T num_read = 0;
			DWORD	target_addr = 0;
			if (!debug_kernel_ptr->read_memory(addr.addr32.offset,&target_addr,4,&num_read) || num_read != 4)
			{
				return NULL;
			}

			if (!debug_kernel_ptr->symbol_from_addr(target_addr,symbol))
			{
				return NULL;
			}
			*symstrlen = symbol.size();
			return symbol.c_str();
		}
		
		return NULL;
	},NULL);
}

CAsmWnd::~CAsmWnd()
{
}


BEGIN_MESSAGE_MAP(CAsmWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEWHEEL()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_CHAR()
	ON_WM_CREATE()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_NOTIFY(HDN_ENDTRACKA, 123, &CAsmWnd::OnHdnEndtrack)
	ON_NOTIFY(HDN_ENDTRACKW, 123, &CAsmWnd::OnHdnEndtrack)
END_MESSAGE_MAP()

BOOL CAsmWnd::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
// 	m_wndOutputWnd.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
// 		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL);
	m_AddrToShow = m_Eip = NULL;
	return __super::Create(NULL,lpszWindowName,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL,
		rect,pParentWnd,nID);
}


// CAsmView 消息处理程序

void CAsmWnd::OnPaint()
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
		CPen pen(PS_SOLID,1,0x00A0A0A0);
		CPen* pPenOld = dcMem.SelectObject(&pen);
		pPenOld->DeleteObject();
		dcMem.MoveTo(m_nAddrWidth,20);
		dcMem.LineTo(m_nAddrWidth,rcClient.bottom);
		dcMem.MoveTo(m_nAddrWidth+m_nHexWidth,20);
		dcMem.LineTo(m_nAddrWidth+m_nHexWidth,rcClient.bottom);
		dcMem.MoveTo(m_nAddrWidth+m_nHexWidth+m_nDisasmWidth,20);
		dcMem.LineTo(m_nAddrWidth+m_nHexWidth+m_nDisasmWidth,rcClient.bottom);

		dc.BitBlt(0,0,rcClient.right,rcClient.bottom,&dcMem,0,0,SRCCOPY);
		dcMem.SelectObject(pOldBmp);
		dcMem.SelectObject(pOldFont);
		bmpMem.DeleteObject();
	});

	CBrush brush(0x00FFFFFF);
	dcMem.FillRect(&rcClient,&brush);
	dcMem.SetBkMode(TRANSPARENT);


	if (debug_kernel_ptr == NULL)
	{
		return;
	}


	m_vecAddress.clear();
	m_vecAsm.clear();

	byte buffer[rcClient.bottom/m_nLineHight*15+1];
	SIZE_T num_read;
	if (!debug_kernel_ptr->read_memory(m_AddrToShow,buffer,rcClient.bottom/m_nLineHight*15,&num_read))
	{
		int y = 0;
		for (int i=0;i<rcClient.bottom/m_nLineHight;++i)
		{
			char szUnknownInsn[10];
			sprintf(szUnknownInsn,"%08X",m_AddrToShow+i);
			m_vecAddress.push_back(m_AddrToShow+i);

			RECT rc;
			rc.top = y;
			rc.bottom = y+m_nLineHight;
			rc.left = 0;
			rc.right = m_nAddrWidth;
			dcMem.ExtTextOut(0,y,ETO_CLIPPED,&rc,szUnknownInsn,8,NULL);

			rc.left = m_nAddrWidth;
			rc.right = rc.left+m_nHexWidth;
			dcMem.ExtTextOut(rc.left,y,ETO_CLIPPED,&rc,"???",3,NULL);

			y+=m_nLineHight;
		}

		return;
	}
	
	CPU_ADDR	curAddr = {0};
	curAddr.addr32.offset = (uint32)m_AddrToShow;
	for (unsigned int i=0,j=0;i<num_read;++j)
	{
		x86dis_insn insn = *(x86dis_insn*)m_Decoder.decode(buffer+i,num_read-i,curAddr);
		if ((uint32)m_AddrToShow>curAddr.addr32.offset && (uint32)m_AddrToShow < curAddr.addr32.offset + insn.size)
		{
			break;
		}

		m_vecAddress.push_back(curAddr.addr32.offset);

		auto type = m_Decoder.is_branch(&insn);

		int y = j*m_nLineHight;

		{
			// 绘制被选中的行
			RECT rc;
			rc.left = 0;
			rc.top = y;
			rc.bottom = y+m_nLineHight;
			rc.right = rcClient.right;

			if (curAddr.addr32.offset >= m_dwSelAddrStart && curAddr.addr32.offset <= m_dwSelAddrEnd) // 这一行被选中了
			{
				dcMem.SetBkColor(0x00C0C0C0);
				dcMem.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rc,NULL,NULL,NULL);
			}
		}

		{
			// 绘制地址.
			RECT rc;
			rc.top = y;
			rc.bottom = y+m_nLineHight;
			rc.left = 0;
			rc.right = m_nAddrWidth;

			char szAddr[10];
			sprintf(szAddr,"%08X",curAddr.addr32.offset);

			debug_kernel::breakpoint_t* bp;
			dcMem.SetTextColor(0x00000000);
			if (curAddr.addr32.offset == (uint32)m_Eip)	// EIP在这一行
			{
				dcMem.SetBkColor(0x00000000);
				dcMem.SetTextColor(0x00FFFFFF);
				dcMem.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rc,NULL,NULL,NULL);
			}
			else if (bp = debug_kernel_ptr->find_breakpoint_by_address(curAddr.addr32.offset))  // 这一行有断点
			{
				if (bp->user_enable) // 是否启用了
				{
					dcMem.SetBkColor(0x000000FF);
				}
				else
				{
					dcMem.SetBkColor(0x00990066);
				}
				dcMem.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rc,NULL,NULL,NULL);
			}
			ExtTextOutWithSelection(dcMem,0,y,&rc,szAddr,8);
		}

		{
			// 绘制HEX
			dcMem.SetTextColor(0x00000000);

			RECT rc;
			rc.top = y;
			rc.bottom = y+m_nLineHight;
			rc.left = m_nAddrWidth+20;
			rc.right = m_nAddrWidth + m_nHexWidth;

			char szHex[15*3+1];
			int num = 0;
			for (byte* p=buffer+i;p!=buffer+i+insn.size;++p)
			{
				num += sprintf(szHex+num,"%02X ",*p);
				assert(num<15*3+1);
			}

			dcMem.ExtTextOut(rc.left,y,ETO_CLIPPED,&rc,szHex,num,NULL);
		}

		{
			int nDasmLeft = m_nAddrWidth+m_nHexWidth;

			RECT rc;
			rc.left = nDasmLeft;
			rc.right = rc.left+m_nDisasmWidth;
			rc.top = y;
			rc.bottom = y+m_nLineHight;

			x86dis_str insn_str = {0};
			m_Decoder.str_insn(&insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD,insn_str);

			std::string asm_str;
			int x = 0;
			if (insn_str.prefix[0])
			{
				asm_str += insn_str.prefix;
				dcMem.SetBkColor(0x000000FF);
				dcMem.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rc,NULL,NULL,NULL);
				dcMem.SetTextColor(0x00E037D7);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,insn_str.prefix,strlen(insn_str.prefix));
				//ExtTextOutWithSelection(dcMem,x*m_nFontWidth+m_nMargenWidth,j*m_nLineHight,asm_str.c_str()+x,asm_str.size()-x);
				//x += strlen(insn_str.prefix);
			}

			if (insn_str.opcode[0])
			{
				std::string opc_str(insn_str.opcode);

				COLORREF TextColor = 0x00000000;
				COLORREF BkColor = 0x00000000;

				if (type != x86Analysis::BR_NONE)
				{
					switch (type)
					{
					case x86dis::BR_JMP:
						TextColor = 0x000000FF;
						BkColor = 0x0000FFFF;
						break;
					case x86dis::BR_JCC:
						TextColor = 0x000000CC;
						BkColor = 0x0000FFFF;
						break;
					case x86dis::BR_LOOP:
						TextColor = 0x000000CC;
						BkColor = 0x0000FFFF;
						break;
					case x86dis::BR_CALL:
						TextColor = 0x00000000;
						BkColor = 0x00FFFF00;
						break;
					case x86dis::BR_RET:
						TextColor = 0x00FF00FF;
						BkColor = 0x00FFFF00;
						break;
					}

					rc.left = nDasmLeft+asm_str.size()*m_nFontWidth;
					rc.right = rc.left + opc_str.size()*m_nFontWidth;
					dcMem.SetBkColor(BkColor);
					dcMem.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rc,NULL,NULL,NULL);
				}


				rc.right = rcClient.right;
				dcMem.SetTextColor(TextColor);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,opc_str.c_str(),opc_str.size());
				asm_str += opc_str;
			}

			dcMem.SetTextColor(0x00000000);

			if (insn_str.operand[0][0])
			{
				std::string ope0_str("  ");

				bool bIsCallApi = false;
				do 
				{
					if (type == x86dis::BR_CALL)
					{
						std::string tmp;
						x86_insn_op op = insn.op[0];
						if (op.type == X86_OPTYPE_IMM
							&& !debug_kernel_ptr->symbol_from_addr(op.imm,tmp))
						{
							byte opcode_buffer[16] = {0};
							SIZE_T num_read = 0;
							if (!debug_kernel_ptr->read_memory(op.imm,opcode_buffer,15,&num_read))
							{
								break;
							}

							CPU_ADDR addr = {0};
							addr.addr32.offset = op.imm;
							x86dis_insn target_insn = *(x86dis_insn*)m_Decoder.decode(opcode_buffer,num_read,addr);
							if (m_Decoder.is_branch(&target_insn) != x86dis::BR_JMP)
							{
								break;
							}

							const char* target_str = m_Decoder.strf(&target_insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD,DISASM_STRF_SMALL_FORMAT);
							ope0_str += "<";
							ope0_str += target_str;
							ope0_str += ">";
							bIsCallApi = true;
							break;
						}
					}
				} while (0);

				if (!bIsCallApi)
				{
					ope0_str += insn_str.operand[0];
				}

				rc.left = nDasmLeft+asm_str.size()*m_nFontWidth;
				//dcMem.ExtTextOut(rc.left,rc.top,ETO_CLIPPED,&rc,ope0_str.c_str(),ope0_str.size(),NULL);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,ope0_str.c_str(),ope0_str.size());
				asm_str += ope0_str;
			}
			if (insn_str.operand[1][0])
			{
				std::string ope1_str(",");
				ope1_str += insn_str.operand[1];

				rc.left = nDasmLeft+asm_str.size()*m_nFontWidth;
				//dcMem.ExtTextOut(rc.left,rc.top,ETO_CLIPPED,&rc,ope1_str.c_str(),ope1_str.size(),NULL);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,ope1_str.c_str(),ope1_str.size());
				asm_str += ope1_str;
			}
			if (insn_str.operand[2][0])
			{
				std::string ope2_str(",");
				ope2_str += insn_str.operand[1];

				rc.left = nDasmLeft+asm_str.size()*m_nFontWidth;
				//dcMem.ExtTextOut(rc.left,rc.top,ETO_CLIPPED,&rc,ope2_str.c_str(),ope2_str.size(),NULL);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,ope2_str.c_str(),ope2_str.size());
				asm_str += ope2_str;
			}
			if (insn_str.operand[3][0])
			{
				std::string ope3_str(",");
				ope3_str += insn_str.operand[1];

				rc.left = nDasmLeft+asm_str.size()*m_nFontWidth;
				//dcMem.ExtTextOut(rc.left,rc.top,ETO_CLIPPED,&rc,ope3_str.c_str(),ope3_str.size(),NULL);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,ope3_str.c_str(),ope3_str.size());
				asm_str += ope3_str;
			}
			if (insn_str.operand[4][0])
			{
				std::string ope4_str(",");
				ope4_str += insn_str.operand[1];

				rc.left = nDasmLeft+asm_str.size()*m_nFontWidth;
				//dcMem.ExtTextOut(rc.left,rc.top,ETO_CLIPPED,&rc,ope4_str.c_str(),ope4_str.size(),NULL);
				ExtTextOutWithSelection(dcMem,rc.left,rc.top,&rc,ope4_str.c_str(),ope4_str.size());
				asm_str += ope4_str;
			}
		}
		
		i += insn.size;
		curAddr.addr32.offset += insn.size;

	}
}

BOOL CAsmWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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

void CAsmWnd::SetEIP( DWORD eip )
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

void CAsmWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (!debug_kernel_ptr)
	{
		return;
	}

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

void CAsmWnd::PreviousCode( DWORD dwTargetAddr,DWORD* pdwPreInsn )
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

void CAsmWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();

	if (point.x>m_nMargenWidth)
	{
		m_bLButtonDown = TRUE;

		int index = point.y/m_nLineHight;

		if (index<m_vecAddress.size())
		{
			m_dwSelAddrEnd = m_dwSelAddrStart = m_vecAddress[index];
		}

		if (index<m_vecAsm.size())
		{
			const std::string& line_str = m_vecAsm[index];
			int pos = (point.x-m_nMargenWidth)/m_nFontWidth;
			int start = 0,end = 0;
			if (debug_utils::get_word_from_pos(line_str,pos,start,end))
			{
				m_strSelWord = line_str.substr(start,end-start);
			}
		}

#if 0
		if (index<m_vecAddress.size() && index<m_vecAsm.size())
		{
			m_dwSelAddrEnd = m_dwSelAddrStart = m_vecAddress[index];
			const std::string& line_str = m_vecAsm[index];
			int pos = (point.x-m_nMargenWidth)/m_nFontWidth;
			int start = 0,end = 0;
			if (debug_utils::get_word_from_pos(line_str,pos,start,end))
			{
				m_strSelWord = line_str.substr(start,end-start);
			}
		}
#endif
		Invalidate(FALSE);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CAsmWnd::OnLButtonUp(UINT nFlags, CPoint point)
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

void CAsmWnd::OnMouseMove(UINT nFlags, CPoint point)
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

void CAsmWnd::UpdateScrollInfo()
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

void CAsmWnd::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
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

int CAsmWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	RECT rc;
	GetClientRect(&rc);
	rc.bottom = 20;
	m_Header.Create(HDS_BUTTONS,rc,this,123);

	SetPaintFont(app_cfg.font_cfg.asm_view_font);

	HDITEM	item = {0};
	item.mask = HDI_WIDTH | HDI_TEXT;
	m_nAddrWidth = item.cxy = m_nFontWidth*8;
	item.pszText = "地址";
	m_Header.InsertItem(0,&item);
	m_nHexWidth = item.cxy = m_nFontWidth*15;
	item.pszText = "HEX";
	m_Header.InsertItem(1,&item);
	m_nDisasmWidth = item.cxy = (rc.right-m_nAddrWidth-m_nHexWidth)/2;
	item.pszText = "反汇编";
	m_Header.InsertItem(2,&item);
	m_nCommicWidth = item.cxy = 99999;
	item.pszText = "注释";
	m_Header.InsertItem(3,&item);
	m_Header.ShowWindow(SW_SHOW);

	m_Header.SetFont(&m_Font);

	return 0;
}

BOOL CAsmWnd::ExtTextOutWithSelection( CDC& dc, int x, int y, LPCRECT lpRect, LPCTSTR lpszString, UINT nCount )
{
	RECT rc;

	if (m_strSelWord.size()>0)
	{
		const char* pSub = lpszString;

		pSub = strstr(lpszString,m_strSelWord.c_str());
		while (pSub)
		{
			if (pSub != lpszString && !debug_utils::is_bound(pSub[-1]))
			{
				break;
			}
			if (pSub+m_strSelWord.size() != lpszString+nCount && !debug_utils::is_bound(pSub[m_strSelWord.size()]))
			{
				break;
			}

			rc.left = x+(pSub-lpszString)*m_nFontWidth;
			rc.right = rc.left+strlen(m_strSelWord.c_str())*m_nFontWidth;
			rc.top = y;
			rc.bottom = y+m_nLineHight;
			dc.SetBkColor(0x00666666);
			dc.ExtTextOut(NULL,NULL,ETO_OPAQUE,&rc,NULL,0,NULL);
			pSub = strstr(pSub+m_strSelWord.size(),m_strSelWord.c_str());
		} 
	}
	
	return dc.ExtTextOut(x,y,ETO_CLIPPED,lpRect,lpszString,nCount,NULL);
}

void CAsmWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	RECT rc = {0};
	GetClientRect(&rc);
	m_Header.MoveWindow(0,0,rc.right,20);

	HDITEM item = {0};
	item.mask = HDI_WIDTH;
	m_Header.GetItem(2,&item);
	m_nDisasmWidth = item.cxy = (rc.right-m_nAddrWidth-m_nHexWidth)/2;
	m_Header.SetItem(2,&item);

	m_Header.GetItem(3,&item);
	m_nDisasmWidth = item.cxy = 99999;
	m_Header.SetItem(3,&item);
}

void CAsmWnd::OnHdnEndtrack( NMHDR *pNMHDR, LRESULT *pResult )
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
		m_nDisasmWidth = pitem->cxy;
		break;
	case 3:
		m_nCommicWidth = pitem->cxy;
		break;
	}

	Invalidate(FALSE);
}

BOOL CAsmWnd::SetPaintFont( const LOGFONT& font )
{
	m_Font.Detach();

	if (! m_Font.CreateFontIndirect(&font))
	{
		return FALSE;
	}

	CDC* pDC = GetDC();
	CFont* pFont = pDC->SelectObject(&m_Font);
	pFont->DeleteObject();
	TEXTMETRIC	text_metrit = {0};
	if (!pDC->GetTextMetrics(&text_metrit))
	{
		return FALSE;
	}
	;
	// 默认行高
	m_nLineHight = text_metrit.tmHeight + text_metrit.tmExternalLeading;
	// 默认字体宽度
	m_nFontWidth = text_metrit.tmAveCharWidth;
	ReleaseDC(pDC);

	return TRUE;
}
